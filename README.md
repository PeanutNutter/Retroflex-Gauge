# Retroflex-Gauge
An Ethanol% compensating afr/lambda/eth% display based on arduino nano + TM1637 seven segment display (optional RGB LED for rich-lean indicator). Has 4 modes with one button to cycle: 1. E0 based afr 2. Eth% based afr 3. lambda 4. eth%.


Pinouts-
  Nano:
    A1 --- Wideband 0-5v input
    D2 --- 7 segment CLK
    D3 --- 7 segment DIO
    D4 --- RGB Red output
    D5 --- RGB Green output
    D6 --- RGB Blue output
    D7 --- Button input
    D8 --- Ethanol Input (50-150hz)
