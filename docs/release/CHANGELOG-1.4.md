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

## Not changed

- **HW301:** the dongle sets its own cluster URL in its image (v1166: no `showCompass`). The
  compass switch does not reach it.

## Documentation

- [altscreen-mhi2q](../deploy/altscreen-mhi2q.md): the compass switch is added to the cluster
  URL switch table.

## Updating from 1.3.1

Copy both `carplay/` **and** `mod/` to the card, because the menu has a new button. Run
**M.I.B. -> Advanced Settings -> Run Custom Script** once, then reboot. The menu's
**Install / update CarPlay** button updates the program files but not the menu, so the new
button appears only after the M.I.B. run.
