# RoFI OS

RoFI moduly potřebují management uživatelských programů a způsob jak je správně
nakonfigurovat. Kromě toho mají moduly "vitální služby" (např. síť).

Co potřebujeme je, aby RoFI fungoval v následujících módech:

- mód vzdáleného ovládání. Externí entita umí vyčítat a interagovat se všemi
  prvky RoFI HALu.
- mód, kdy je vykonáván uživatelský program. Program je třeba umět monitorovat,
  pozastavovat, spouštět.

Pro jednoduchovat navrhujeme umístit všechny součásti RoFIOS do nativního
programu (napsaného v C++) a všechny uživatelské programy přesunou do
Javascriptu - tj. Jacula. Bylo by hezké umět spouštět i nativní programy, ale to
bude vyšší dívčí. Každopádně pokud by se nám podařilo rovnou navrhnout způsob,
jak reprezentovat uživatelské programy nativně (např. jako FreeRTOS task), mohl
by být JS program spojen s Jaculem jako nativní task.

Dálkové ovládání by mělo poslouchat na zprávy. Tyto zprávy můžou být přijímány
po různých kanálech; zejména socketu, sériové lince a BT GAP protokolu.
Nejzajímavější a nejpriotnější je zprovoznění BT.

Bluetooth protokol funguje následovně. Veškerá komunice se děje broadcastováním
skrze protokol GAP. Není třeba ustavit žádné spojení, protože je možné, že
bychom rychle narazili na limit spojení (dle rychlé rešerše to vypadá na 4-5
spojení - pokud to není pravda, je vhodné ustavit spojení a šifrování).

Po celou dobu robot broadcastuje svou existenci - své unikátní ID a lidsky
čitelné jméno (možná je možné použít klasický BT brokol?). Jakmile má server k
dispozici ID klientů, může jim začít posílat ovládací zprávy. Ovldádací zprávy
jsou buď Google Protobufy nebo JSONy. Ovládací zprávy se opět broadcastují a
roboti ví, že zpráva je určena pro něj podle ID ve zprávě.

Rozhraní pro ovládání robota musí poskytovat zprávy pro ovládání HALu, ovládání
uživatelského programu a nastavení bezdrátové sítě (WIFI auth a auth WLAN FI).

# Konfigurační aplikace

Pro obsluhu BT protokolu je chtěno mít aplikaci pro počítač, která umí
vylistovat spuštěné roboty a přes GUI je ovládat (nakonfigurovat síť, pustit
program, ovládat klouby, vyčítat statistiky). Aplikace je napsána v něčem
příčetném; např. Python nebo obodobná technologie.

# Co je třeba

- vyrobit Jaculus driver pro RoFI HAL,
- implementovat orchestraci spouštění uživatelských programů,
- implementovat BT protokol,
- implementovat desktopovou aplikaci,

A pak by bylo ještě hezké doplnit nějaké nedodělané aspekty univerzálního modulu:

- implementovat nějaké chybějící části (např. obsluha display) v koprocesoru
- implementovat rozumné rozhraní mezi ESP32 a koprocesorem

A úplně žůžo:

- přes BT rozhraní napsat ROS node pro RoFI moduly
