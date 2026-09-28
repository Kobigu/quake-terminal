# Quake Terminal — custom kitty fork for COSMIC

A DD-Term-style drop-down terminal for the COSMIC desktop, built exactly as
requested: **press F12 and it slides down from the top of the screen, takes up
half the screen, and does everything a regular terminal does — including
displaying images.** It runs on **our own custom-built kitty** (a fork of kitty
0.49.1), never on the preinstalled kitty.

```
F12  →  terminal slides down from the top (half screen, full width)
F12  →  slides back up and releases the keyboard
```

Everything below lives in `~/quake-terminal/` on the target machine.

## Repository layout / fresh clone

This repo contains the integration layer; the kitty fork itself is the `kitty/`
submodule (repo `quake-kitty`, branch `quake`, based on upstream `v0.49.1`).
To deploy on a new COSMIC machine:

```bash
git clone --recursive <this-repo-url> ~/quake-terminal
cd ~/quake-terminal

# build the fork (needs the build deps in the README section below)
./build.sh linux-package

# install
ln -sf ~/quake-terminal/quake-terminal ~/.local/bin/quake-terminal
ln -sf ~/quake-terminal/kitty/linux-package/bin/kitty ~/.local/bin/kitty
ln -sf ~/quake-terminal/kitty/linux-package/bin/kitten ~/.local/bin/kitten
mkdir -p ~/.config/autostart && cp deploy/quake-terminal.desktop ~/.config/autostart/
sudo cp deploy/keyd.conf /etc/keyd/default.conf && sudo systemctl enable --now keyd
cp deploy/cosmic-shortcuts.ron ~/.config/cosmic/com.system76.CosmicSettings.Shortcuts/v1/custom
```

Not in git (machine-local or too big): `kitty/linux-package/` (build output),
`docs-venv/` (sphinx venv for the man/html pages), `tools-bin/` (downloaded
slang shader compiler), `*.log`.


## How it works

```
F12 (physical key)
  → keyd (evdev hotkey daemon, /etc/keyd/default.conf)
  → /home/toby/.local/bin/quake-terminal        (toggle wrapper, debounced)
  → kitten quick-access-terminal --config …     (our fork)
  → kitty panel window (Wayland layer-shell surface)
```

- The terminal is a **layer-shell overlay window** docked to the top edge at
  50% of the monitor height (`lines 50%` — a fork feature, see below). It sits
  above normal windows and never appears in the taskbar or alt-tab.
- The instance is **pre-launched hidden at login** (`~/.config/autostart/
  quake-terminal.desktop` → `quake-terminal-preload`), so F12 is instant.
- The shell session **persists** across toggles; only the window is hidden.
- **Images**: full kitty graphics protocol support — `kitten icat file.png`
  works (verified end-to-end on this machine).

## The fork (`~/quake-terminal/kitty`, branch `quake`)

Custom version of kitty v0.49.1 (`kitty --version` → `0.49.1 (quake fork)`),
built from source. Differences from upstream (all marked `quake fork` in the
source):

1. **`lines 50%` / `columns 50%`** — sizes may be given as a percentage of the
   monitor, resolved at layer-size calculation time (so it's exact on any
   monitor/scale).
2. **`slide_duration`** (default 150 ms) — the layer surface slides in/out
   from its docked edge by animating the layer-shell margin
   (`zwlr_layer_surface_v1.set_margin`), with an ease-out curve.
3. **Reliable hide/show on compositors like cosmic-comp** — upstream kitty's
   quick-access hide (null-buffer unmap) can never be shown again on
   cosmic-comp/niri, because those compositors never re-configure a remapped
   layer surface. The fork instead:
   - *hide*: slides out, then destroys the layer surface (this also releases
     the keyboard — cosmic-comp only applies keyboard interactivity at map
     time and ignores runtime changes);
   - *show*: destroys and re-creates the whole `wl_surface` (plus viewport,
     fractional-scale, EGL window and **EGLSurface** — the GL context and all
     its resources survive) and creates a fresh layer surface, which gets a
     fresh configure and fresh keyboard interactivity on every compositor.
   - a guard in `commit_window_surface()` never commits a surface whose
     layer-surface role was destroyed (a protocol error on cosmic-comp).
4. **`listen_on` option** for the quick-access kitten so the instance has a
   stable remote-control socket (`unix:@quake-kitty`).
5. Fix for an upstream bug in the quick-access Go wrapper (`--detached-log`
   was passed as a bare positional and got swallowed into the child command).
6. Version branding (`(quake fork)`).

## Files

| path | purpose |
|---|---|
| `~/quake-terminal/kitty/` | the fork (git repo, branch `quake`) |
| `~/quake-terminal/kitty/linux-package/` | the built, self-contained install |
| `~/quake-terminal/quick-access-terminal.conf` | behavior/geometry config |
| `~/quake-terminal/quake-terminal` | F12 toggle wrapper (also at `~/.local/bin/quake-terminal`) |
| `~/quake-terminal/quake-terminal-preload` | login preloader (hidden) |
| `~/quake-terminal/autostart/quake-terminal.desktop` | installed to `~/.config/autostart/` |
| `~/quake-terminal/build.sh` | rebuild script (deps shims included) |
| `/etc/keyd/default.conf` | F12 binding (keyd) |
| `~/.config/cosmic/com.system76.CosmicSettings.Shortcuts/v1/custom` | F12 → wrapper (compositor-side fallback after reboot) |
| `~/.local/bin/kitty`, `~/.local/bin/kitten` | symlinks — our build is the default `kitty`/`kitten` |
| `/tmp/quake-terminal.log` | debug log of the running instance |

## Configuration

Everything tweakable lives in `~/quake-terminal/quick-access-terminal.conf`:

- `lines 50%` — height (percent, `px`, or rows)
- `slide_duration 150` — animation ms (0 = instant)
- `layer overlay` — draw above the top panel (`top` = below it)
- `focus_policy exclusive` — keyboard is grabbed while open (quake-style)
- DD-Term-style auto-hide when clicking elsewhere: set
  `focus_policy on-demand` + `hide_on_focus_loss yes`
- `kitty_conf kitty.conf` — uses your normal `~/.config/kitty/kitty.conf`
  (your Oxidized Copper theme, Fira Mono, 0.86 opacity etc. all apply)

After editing the conf, just press F12 twice (hide + show) — the running
instance picks up config changes on every toggle.

## Rebuilding after source changes

```bash
~/quake-terminal/build.sh linux-package
```

The build needs the pkg-config/x11-xcb shims in `~/quake-terminal/pkgconfig-shims/`
(Debian dropped `x11-xcb.pc`), `slangc` in `~/quake-terminal/tools-bin/`, and
the Symbols Nerd Font in `~/.local/share/fonts/` — all already in place.

To rebase onto a new kitty release: `git fetch --tags && git checkout v0.49.x
&& git checkout quake && git rebase v0.49.x`, resolve conflicts (all fork
changes are marked `quake fork`), rebuild.

## Why keyd (and not just a COSMIC custom shortcut)?

The custom-shortcut entry *is* in place (file above) and will work after a
reboot — cosmic-comp only reads that file at startup, so a live edit can't
reach the running compositor, and restarting cosmic-comp was off the table.
`keyd` binds F12 at the evdev level *now* (and consumes the key before the
compositor, so there's no double-fire; the wrapper is additionally debounced).
If keyd is ever removed, the compositor binding takes over on next login.

## The old setup (replaced)

The previous `cosmic-ext-quake-terminal` (which just minimized/raised a kitty
window) was stopped and its F12 shortcut repointed. To fully remove it:

```bash
sudo rm /usr/bin/cosmic-ext-quake-terminal
rm -rf ~/.config/cosmic/com.github.m0rf30.CosmicExtQuakeTerminal
rm -f ~/.local/share/applications/com.github.m0rf30.CosmicExtQuakeTerminal.desktop
```

A backup of the original shortcuts file is at
`~/.config/cosmic/com.system76.CosmicSettings.Shortcuts/v1/custom.pre-quake-fork.bak`.

## Remote control

The instance listens on `unix:@quake-kitty`:

```bash
kitten @ --to unix:@quake-kitty ls
kitten @ --to unix:@quake-kitty send-text "echo hi
"
kitten @ --to unix:@quake-kitty get-text --extent=screen
```

## Uninstall

```bash
sudo systemctl disable --now keyd && sudo apt remove keyd
rm -f ~/.local/bin/{quake-terminal,kitty,kitten}
rm -f ~/.config/autostart/quake-terminal.desktop
rm -rf ~/quake-terminal
# restore the old shortcut if you want it back:
cp ~/.config/cosmic/com.system76.CosmicSettings.Shortcuts/v1/custom{.pre-quake-fork.bak,}
```

## Notes / gotchas

- The slide direction follows the docked edge (`edge top` → slides down).
- Multi-monitor: the terminal appears on the output the compositor picks
  (usually the one you interacted with); `output_name` can pin it.
- Compositor support for runtime keyboard-interactivity changes is not
  universal; the fork's destroy/re-create show path works on cosmic-comp,
  sway, Hyprland and KDE (it only ever uses standard protocol objects).
- Everything was tested live on this machine: spawn/hide/show cycles, rapid
  toggling, keyboard grab/release, real F12 keypresses at the evdev level,
  and image display via `kitten icat`. cosmic-comp was never restarted or
  crashed (same PID since login).
