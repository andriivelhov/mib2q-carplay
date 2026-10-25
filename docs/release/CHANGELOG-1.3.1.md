# mib2q-carplay 1.3.1 - changelog

Changes since [1.3](CHANGELOG-1.3.md).

## New

- **Turn card, ETA and speed limit on the CarPlay cluster map switch ON / OFF in the menu.**
  Three new Green Engineering Menu buttons:
  - *Cluster map turn card ON / OFF* (default **ON**)
  - *Cluster map ETA ON / OFF* (default **OFF**)
  - *Cluster map speed limit ON / OFF* (default **OFF**)

  Each button writes a value file in `/mnt/app/root/` (`mibr-carplay111-nav.showManeuver`,
  `.showETA`, `.showSpeedLimit`; absent = default). With a CarPlay cluster session up, the press
  re-sends the cluster map request at once (`/tmp/mibr-alt111-show-only`); otherwise it applies at
  the next connection. `showSpeedLimit` also accepts `user` (follow the iPhone's Maps setting) when
  the file is written by hand. URLs:
  - turn card ON: `maps:/car/instrumentcluster?showETA=…&showSpeedLimit=…`
  - turn card OFF: `maps:/car/instrumentcluster/map?showETA=…&maneuverLayout=no&showSpeedLimit=…`

## Changed

> [!IMPORTANT]
> **Using the car's own (native) maneuver arrow? Turn the phone's turn card off by hand.**
> 1.3.1 no longer hides the Maps turn card automatically when the cluster draws its own arrow.
> To keep only the native arrow, open **Green Engineering Menu -> CarPlay-RGI -> Cluster map
> turn card ON / OFF** and switch it **OFF** once. The setting is kept across reboots.

- **The cluster map request no longer follows route guidance.** In 1.3 the head unit asked for
  the bare map while our own arrow was shown and for the app's turn card and ETA otherwise. The
  three switches above now decide, whatever the guidance source. With the defaults the phone's
  turn card is also shown while the cluster draws our arrow; switch the turn card OFF for the 1.3
  look with route guidance. Java still sends `CMD_ALT_RGI`; `altscreen111_set_rgi` only logs it.
  All existing URL overrides (`/tmp/mibr-alt111-url-mode`, `mibr-carplay111-nav.surface`,
  `mibr-carplay111-url-map.enabled`, `mibr-carplay111-nav-query.enabled`,
  `mibr-carplay111-rgi-url.disabled`, `ALTSCREEN111_PARITY_URL`) still win.

## Not changed

- **HW301:** the dongle sets its own cluster URL in its image (v1166: turn card, no ETA, no speed
  limit). The new switches do not reach it.

## Documentation

- [altscreen-mhi2q](../deploy/altscreen-mhi2q.md): the three switches and their URLs replace the
  route-guidance URL table. README AltScreen feature line updated.

## Updating from 1.3

Copy both `carplay/` **and** `mod/` to the card, because the menu has three new buttons. Run
**M.I.B. -> Advanced Settings -> Run Custom Script** once, then reboot. The menu's
**Install / update CarPlay** button updates the program files but not the menu, so the new
buttons appear only after the M.I.B. run.
