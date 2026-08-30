# USB-C-/PoE-Stromquellenumschaltung / USB-C/PoE power ORing

## Deutsch

USB und PoE werden bereits auf ihrer jeweiligen Quellplatine entkoppelt, bevor
sie auf `SYS_5V` zusammentreffen. `USB_5V` liegt nur an USB-C J2 an,
`POE_5V_RAW` nur am DP1435-Ausgang der PoE-Tochterplatine. `SYS_5V` versorgt
AP2112K, Eingangskondensator und Stack-Pin 9.

Zwei einzeln ausreichend bemessene Schottky-Dioden vom Typ **PMEG2010EA** im
SOD-323-Gehäuse sind verbindlich:

- D3 der Hauptplatine: Anode `USB_5V`, Kathode `SYS_5V`
- D3 der Tochterplatine: Anode `POE_5V_RAW`, Kathode `SYS_5V`

Es darf keinen Überbrückungsjumper geben. Eine Brücke über eine der Dioden würde
die Rückspeisesperre aufheben und wäre bei gleichzeitigem USB-C und PoE
gefährlich. Die LAN-only-Bestückung lässt den vollständigen PoE-Ausgang
unbestückt; die Hauptplatine funktioniert weiterhin allein über USB-C.

## English

## Implemented separated power paths

USB and PoE are isolated at their respective source boards before they meet on `SYS_5V`.

| Net | Source / destination |
|---|---|
| `USB_5V` | USB-C J2 VBUS pins only |
| `POE_5V_RAW` | DP1435 output on the PoE daughterboard only |
| `SYS_5V` | AP2112K input, enable and input capacitor |
| `SYS_5V` on J1 pin 9 | Already-isolated PoE feed from the daughterboard |

Use two individually rated Schottky diodes:

- D3: PMEG2010EA, anode `USB_5V`, cathode `SYS_5V`
- Daughterboard D3: PMEG2010EA, anode `POE_5V_RAW`, cathode `SYS_5V`

Both parts use `Diode_SMD:D_SOD-323`. PMEG2010EA (1 A, 20 V) provides enough current
margin for the AP2112K's 600 mA maximum output and prevents either external
source from feeding current back into the other source.

Main-board D3 is on the component side. The PoE diode is also on the component
side of the daughterboard. There is no PoE diode on the main-board underside.

No bypass jumper is fitted. A jumper across either diode would defeat source
isolation and would be unsafe when USB-C and PoE are connected simultaneously.
Source selection is automatic and requires no user action.

The PoE daughterboard drives stack pin 9 (`SYS_5V`) only when its DP1435 PoE
population, including daughterboard D3, is fitted. The LAN-only population
leaves the complete PoE output path unpopulated. The main board remains fully
functional from USB-C without either daughterboard variant.
