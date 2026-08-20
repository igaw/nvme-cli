# NVMe Privilege Separation

*Status: draft design plan. Nothing described here is implemented yet.*

A plan for isolating the root-privileged ioctl and `/dev/nvme-fabrics` I/O from
the unprivileged parsing, decoding, and display code that surrounds it,
shippable additively, after 3.0, with no API or ABI break.

This is a different split from the one in [Integrating libnvme into an
application](INTEGRATION.md). That document draws the line between the library
and its caller; this one draws a second line inside the library's own side,
between the code that talks to the kernel and the code that parses what comes
back.

## Motivation

nvme-cli runs one process, at full root, for the entire lifetime of a command,
including the parts of it that were never meant to be trusted.

Today, argument parsing, config-ini parsing, JSON/stdout formatting, and every
vendor plugin's response-buffer decoding all execute in the same process, at the
same privilege, as the actual `ioctl()` calls that talk to the drive. That
process is usually root. A parsing bug anywhere in that surface, triggered by a
crafted Identify or Log Page response from a malicious or malfunctioning drive,
or just an ordinary buffer-handling mistake, runs with full root privilege, not
the privilege the bug actually needs.

This is not hypothetical. Commit `d5aa1d7d4` on this repo, "solidigm: fix
STRING_OVERFLOW in join_options", is exactly this class of bug: a buffer issue
in command-line/display string construction, caught by static analysis. That
code has no business running as root. It just happens to, because it lives in
the same process as the code that does.

The fix is a trust boundary, not more code review. Move everything that touches
attacker- or drive-supplied bytes for the purpose of decoding, formatting, or
displaying them out of the root process, and confine root to the minimum surface
that actually needs it: issuing the passthru ioctl, and reading/writing
`/dev/nvme-fabrics`.

## Non-goals

- **Not a defense against an authorized malicious user.** Anyone who can reach
  the helper can still issue `sanitize`, `format`, `security-send`, or firmware
  activation — that power is inherent to what nvme-cli is for. This plan narrows
  which code needs root, not what root can do.
- **Not a persistent daemon.** nvme-cli is one-shot; the design below forks a
  privileged helper per invocation and tears it down on exit, rather than
  standing up a long-lived system service.
- **Not a rewrite of the ~50 vendor plugins.** Two files bypass the abstraction
  and need direct attention (see Phase 5); the rest need zero changes.

## Current state

The privileged surface is already narrow. It just isn't isolated.

All admin/I/O passthru commands, from every plugin, funnel through two
functions: `libnvme_exec_admin_passthru` and `libnvme_exec_io_passthru` in
`libnvme/src/nvme/ioctl-linux.c` (lines 233-325). The raw `ioctl()` calls
themselves are confined to `ioctl_passthru32`/`ioctl_passthru64` in that same
326-line file. Fabrics connect is smaller still: `open("/dev/nvme-fabrics",
O_RDWR)` and `write(fd, argstr, len)`, in `libnvme/src/nvme/fabrics.c` (lines
1564-1573). Discovery-log content arrives over the same admin-passthru path once
connected, so it needs no separate handling.

A repo-wide search of `src/` and `plugins/` found no plugin or CLI code calling
`ioctl()` on the NVMe device directly, with two exceptions:

- **`plugins/sed/sedopal_cmd.c`** (privileged today): SED Opal ioctls
  (`IOC_OPAL_TAKE_OWNERSHIP`, `IOC_OPAL_LOCK_UNLOCK`, `IOC_OPAL_REVERT_TPR`,
  ...) called directly on the fd via `libnvme_transport_handle_get_fd()`.
- **`plugins/scaleflux/sfx-nvme.c`** (privileged today): vendor and block-layer
  ioctls (`SFX_GET_FREESPACE`, `NVME_IOCTL_CLR_CARD`, `BLKGETSIZE64`, ...), same
  pattern.

Everything else, the ~26k lines of libnvme core, and the considerably larger
volume of plugin code, only ever calls into the libnvme API. Vendor plugins
routinely interleave a passthru call with immediate
`printf`/`json_object_add_value_*` formatting of the result in the same function
(`plugins/wdc/wdc-nvme.c` is representative, at over 5,000 lines), but none of
that code touches a raw fd itself. That single fact is what makes this plan
tractable: the split can happen underneath the plugins, not across them.

**The chokepoint already exists.** `struct libnvme_transport_handle`
(`libnvme/src/nvme/private.h`, lines 178-229) already carries a `type` enum,
`DIRECT`, `MI`, `LOOPBACK`, and the two exec functions above switch on it to
decide how to actually move bytes. Adding a fourth type is an extension of a
pattern that's already load-bearing, not new architecture.

## Target architecture

nvme-cli is a one-shot CLI, not a service, so the right pattern is the classic
privilege-separation fork used by OpenSSH, sudo, and dhclient, not a system
daemon. At startup, while still privileged, create a `socketpair(AF_UNIX,
SOCK_SEQPACKET)` and fork:

```
        Parent: everything else               Child: ioctl server
        ------------------------               --------------------
        - argument & config-ini parsing        - validates device path
        - builds passthru command structs        against an allowlist
        - all of nvme-print-*.c                - drops all capabilities
        - every vendor plugin's decode/          but CAP_SYS_ADMIN
          print logic                          - reads one fixed-format
        - drops to real uid/gid                  request at a time
          immediately after fork                - issues exactly the
                                                   ioctl or connect write
                    <-- socketpair -->          - returns raw result
                    (SOCK_SEQPACKET)              bytes, uninterpreted
                                                          |
                                                          v
                                            Linux kernel: NVMe char
                                            device ioctl,
                                            /dev/nvme-fabrics
                                            read/write
```

The wire protocol is a fixed-size struct mirroring `libnvme_passthru_cmd` plus a
length-capped data buffer (sized to the existing transfer limits already used
for telemetry/log pages, e.g. `NVME_LOG_PAGE_PDU_SIZE`), sent over the
pre-created socket. No file descriptor passing, no second privileged `open()`
after the device is first validated and opened.

**Hardening the child:**

- Device path checked against `^/dev/nvme[0-9]+(n[0-9]+)?$` or exactly
  `/dev/nvme-fabrics`, opened `O_NOFOLLOW`, once, before the seccomp filter is
  installed.
- Capabilities dropped to `CAP_SYS_ADMIN` only (the kernel's NVMe admin passthru
  requirement) immediately after that open.
- A seccomp-bpf filter restricts the child to `{ioctl, read, write, close,
  exit_group}` after setup, filtered further on the specific `LIBNVME_IOCTL_*`
  request codes where seccomp's argument comparison allows it.

## API and ABI compatibility

Every type this touches is already opaque to callers. That's what makes this
safe to defer past 3.0.

`struct libnvme_global_ctx` and `struct libnvme_transport_handle` are
forward-declared only in every public header (`lib-types.h:13-14`,
`loopback.h:13-14`, `config.h:12`, and others); their real definitions live in
`private.h`, which no external consumer includes. Callers only ever hold the
pointer `libnvme_open()` hands back.

- A new `LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP` enum value, and new fields on
  the handle struct, change nothing any caller can see or has laid out
  themselves: no SONAME bump (`libnvme_so_version`,
  `libnvme/src/meson.build:211,222`).
- Opt-in activation follows the existing pattern of `libnvme_set_owner()`,
  `libnvme_set_test_base_dir()`, and `libnvme_set_test_sysfs_dir()` (`lib.h`): a
  new setter on the opaque ctx, called before `libnvme_open()`, additive by
  construction.
- `libnvme_transport_handle_get_fd()` already returns `LIBNVME_INVALID_FD` for
  non-ioctl handle types (MI does this today); a PRIVSEP handle doing the same
  is a third caller of an existing rule, not a new contract. Callers that bypass
  the abstraction (see Current state) degrade gracefully rather than breaking.

**Conclusion:** 3.0 ships as-is. This entire feature can land as additive
symbols in a later 3.x release with no coordinated break for downstream libnvme
consumers beyond nvme-cli.

## Phased plan

Each phase is independently mergeable and leaves the default (non-privsep) path
unchanged until Phase 6.

**Phase 0 — Spike.** No shipped behavior change. Prototype the wire protocol and
a throwaway helper against one plugin family (e.g. Identify + Get Log Page) to
validate the request/response framing and buffer-size caps before committing the
shape in libnvme.

**Phase 1 — libnvme: passthru channel.** Add
`LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP`, the marshal/unmarshal code, and a
standalone helper binary covering direct-attached admin/I/O passthru only, the
`ioctl-linux.c` surface. No fabrics, no plugin changes.

**Phase 2 — libnvme: fabrics channel.** Route the `open("/dev/nvme-fabrics")` +
`write(argstr)` connect sequence through the same helper. `build_options()`'s
string construction stays in the unprivileged side; only the final string
crosses the boundary.

**Phase 3 — nvme-cli: process lifecycle.** Wire `main()` to fork + socketpair at
startup when installed privileged, drop to the real uid/gid in the parent
immediately after, and fall back to the current direct-ioctl path when not
installed with elevated privilege, so nothing regresses for the common "already
running as root" case.

**Phase 4 — Harden the helper.** Path allowlist, `O_NOFOLLOW`, capability drop
to `CAP_SYS_ADMIN`, seccomp filter. This is the phase that earns the design its
name; treat the helper's diff as requiring the highest review bar in the
project.

**Phase 5 — Close the two exceptions.** Add request opcodes for the SED Opal
ioctls (`sedopal_cmd.c`) and the ScaleFlux/block ioctls (`sfx-nvme.c`), and
switch those two files from `libnvme_transport_handle_get_fd()` + raw `ioctl()`
to the new channel. Every other plugin needs no change, the whole point of
interposing at the transport-handle layer.

**Phase 6 — Packaging and rollout.** Default stays off. Decide setcap-on-binary
vs. setuid-root packaging per distro, document the installation choice, add a CI
job that actually exercises the privileged path (needs root in CI), and only
then flip the default for packages that opt in.

## Risks and open questions

- **Per-command IPC overhead.** Expected negligible; nvme-cli is one-shot and
  human-latency-bound; worth a quick benchmark in Phase 0 rather than assuming.
- **Buffer size cap for large transfers.** Telemetry log capture is the outlier
  (can run to megabytes). The socket protocol needs a cap well above
  `NVME_LOG_PAGE_PDU_SIZE` for that path specifically, chosen in Phase 0.
- **setuid vs. file capabilities vs. polkit.** File capabilities on just the
  helper binary are the tighter option (no arbitrary-file root, just
  `CAP_SYS_ADMIN`); needs a packaging decision per distro in Phase 6.
- **CI coverage for the privileged path.** Requires a runner with real device
  access or a kernel-loopback target; the existing loopback transport handle
  type may be reusable for helper testing.
- **Review discipline on the helper.** A few hundred lines that are the entire
  new TCB deserve a standing rule: two reviewers, no feature creep, changes
  justified against the request protocol.

## Definition of done

- No parsing, decoding, JSON/stdout formatting, or config-ini code runs with
  elevated capability.
- The privileged helper's `open()`/`ioctl()`/`write()` targets are the only
  audited root surface in the tree, and stay small enough to review as a unit.
- Zero required changes across the ~48 plugins that only ever call the libnvme
  API; `sedopal_cmd.c` and `sfx-nvme.c` are the two deliberate exceptions,
  closed in Phase 5.
- 3.0 ships unaffected; every symbol this plan adds is new, and no existing
  signature or struct layout visible to a libnvme caller changes.

