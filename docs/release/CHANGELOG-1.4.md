# mib2q-carplay 1.4 - changelog

Changes since [1.3.1](CHANGELOG-1.3.1.md).

## New

- **Compass on the CarPlay cluster map switches ON / OFF in the menu.** New Green Engineering
  Menu button *Cluster map compass ON / OFF* (default **OFF**). It writes
  `/mnt/app/root/mibr-carplay111-nav.showCompass` (absent = default) and applies like the turn
  card, ETA and speed limit buttons: at once with a CarPlay cluster session up, otherwise at the
  next connection. The file also accepts `user` (follow the iPhone's Maps setting) when written by
  hand. Every cluster URL now carries `showCompass`:
  - turn card ON: `maps:/car/instrumentcluster?showETA=…&showSpeedLimit=…&showCompass=…`
  - turn card OFF: `maps:/car/instrumentcluster/map?showETA=…&maneuverLayout=no&showSpeedLimit=…&showCompass=…`

- **All cluster map features on the wide cockpit view only, one menu button.** New Green
  Engineering Menu button *Cluster map all features on wide view ON / OFF* (default **OFF**). ON
  writes `/mnt/app/root/mibr-carplay111-nav.wideAll` with `yes`: the wide view then gets the turn
  card, ETA, speed limit and compass, whatever the four individual switches say. The classic and
  Sport small maps keep using the four switches, which work as before. When the cockpit switches
  between the wide view and a small map, the hook sends `showUI` with the new URL before
  `updateViewArea`. Explicit URL overrides (`mibr-carplay111-nav.surface` and the others listed in
  [altscreen-mhi2q](../deploy/altscreen-mhi2q.md)) still win. *Car test pending.*

- **HW301 image v1167: ETA, speed limit and compass on the wide cockpit layout only.** The
  dongle now picks the cluster URL by cockpit layout:
  - wide: `maps:/car/instrumentcluster?showETA=yes&showSpeedLimit=yes&showCompass=yes`
  - classic and Sport small maps: `maps:/car/instrumentcluster?showETA=no&showSpeedLimit=no&showCompass=no`
    (turn card only, as in v1166)

  When the cockpit switches between wide and a small map, the dongle sends the phone `showUI`
  with the new URL before `updateViewArea`. The URLs are built into the image; the head unit's
  GEM switches still do not reach the dongle. Image `dongle/hw301/fw/hw301_v1167.tar`
  (marker `v1167-20261025192423`). *Car test pending.*

- **HW301 image v1172: the dongle passes the car's cluster settings to the phone.** The
  dongle now behaves like a phone that supports two screens: it takes the cockpit display (size,
  SafeAreas, layouts, start URL) from the car and forwards the car's cluster commands to the
  iPhone. The head unit's settings therefore reach the HW301 too: the five cluster map switches
  (turn card, ETA, speed limit, compass, all features on the wide view), SafeArea changes and the
  wide / classic / Sport layout switch. The image no longer has built-in URLs or SafeAreas. The
  cluster stream has its own connection and encryption, separate from the main screen. To the car
  the dongle reports `model=HW301` with the stock build `18D70`.
  - Made for the **1.4** head unit. It uses the same head-unit path as an iPhone, so 1.3 and
    1.3.1 should work too (not tested, and without the wide-view switch). Update the head unit
    first, then flash the dongle.
  - Image `dongle/hw301/fw/hw301_v1172.tar` (marker `v1172-20261027093548`), now the newest
    HW301 image. `dongle/hw301/fw/hw301_v1173.tar` (marker `v1173-20261027093537`) is the same
    version with diagnostic logging, for bug reports. See [hw301-dongle](../deploy/hw301-dongle.md).
  - First car test on 2026-10-27 with v1173: cluster map on the Virtual Cockpit, CarPlay, Siri,
    calls and navigation prompts on the MMI; the cluster kept running through two main-screen
    hand-overs to the MMI (8,400 frames, none bad, dropped or failed). *Car test pending:* layout
    switching, the menu switches, unplug/replug and phone Wi-Fi loss.
  - The v1167 image keeps working with the 1.4 head unit, unchanged.

## Changed

- **The head unit no longer needs to recognise the HW301.** The v1172 image uses the normal
  two-display path like a phone. The old path for the `iPhone9,1` / `18D70` identity stays for
  v1167, older custom images and stock HW301 firmware, and for senders listed in
  `mibr-carplay111-noadvertise.senders`.
- **More robust stream-111 transport for the old dongle path.** The head unit now waits on that
  socket with `poll()` instead of `select()`, so a high descriptor number no longer stops the
  cluster stream. A stalled view or key frame message times out after 200 ms instead of
  blocking, and these messages also go out while the cluster video is idle.

## Fixed

- **Cluster map ETA missing after a reconnect until the switch was toggled (issue #35).** The
  ETA (and the speed limit and compass) settings reached only the app that drew the cluster at
  connect time. When Waze started guidance later, iOS handed it the cluster with a `suggestUI`,
  which the hook acknowledged but never answered, so Waze drew no ETA although the switch was ON.
  The hook now answers `suggestUI` 1 s later with `showUI` and the current URL, followed by
  `updateViewArea` and `forceKeyFrame`, in the same order as at session start. A burst of
  `suggestUI` gives one `showUI`. `altscreen111.log`: `policy=RESHOW` and
  `gen2 suggestUI -> showUI reshow rc=0`. *Car test pending.*

## Known issues

- **HW301: the start of Siri's answer can be cut.** In the 2026-10-27 test the car took 1.6 to
  1.9 s to open the Siri audio stream, against 0.1 to 0.4 s for media and prompts. The phone
  starts speaking at once, so the first 1.5 to 2 s are lost. Not caused by the v1172 changes; no
  earlier HW301 log contains a Siri session to compare. Under investigation.
- **Short cluster map blinks.** The picture restarts for a moment when iOS restarts its video
  encoder (seen several times when route guidance starts, on both screens at once) and when the
  car asks for a key frame (seen while the MMI showed the Audi menu).

## Documentation

- [altscreen-mhi2q](../deploy/altscreen-mhi2q.md): the compass and the wide-view switches are
  added to the cluster URL switch table; the HW301 v1172 identity next to the legacy one.
- [hw301-dongle](../deploy/hw301-dongle.md), dongle README and FINDINGS: v1167 and its
  per-layout URLs; the v1172 design, its build and the first car test.

## Updating from 1.3.1

Copy both `carplay/` **and** `mod/` to the card, because the menu has new buttons. Run
**M.I.B. -> Advanced Settings -> Run Custom Script** once, then reboot. The menu's
**Install / update CarPlay** button updates the program files but not the menu, so the new
buttons appear only after the M.I.B. run.

HW301 users: update the head unit first. Then flash `fw/hw301_v1172.tar` on the bench, or keep
v1167, which works as before.
