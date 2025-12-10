# AT_Slave application for STM32WL55CCU6

## Please refer to AN5481 to have the complete list of commands

The firmware turns the microcontroller into a LoRaWAN modem controlled via UART. An user can send AT commands to configure LoRaWAN parameters and perform operations.
To test the RF matching performances, the follwing commands can be used.

## AT+TCONF, AT+TTONE, AT+TOFF Commands

| Command     | Syntax Examples                          | Description |
|-------------|------------------------------------------|-------------|
| **AT+TCONF** | `AT+TCONF=868000000:14:4:12:4/5:0:0:1:16:25000:2:3<CR>` | **Sintax**<br>'AT+TCONF=<freq>:<pow>:<bw>:<sf>:<cr>:<lna>:<pa>:<mod>:<paylen>:<freqdev>:<lowdropt>:<BT><CR>'**Response:** `+TCONF: <mode>,<src>,<interval>` then `OK` [web:4] |
| **AT+TTONE** | `AT+TTONE?\r`<br>`AT+TTONE=1,1000,500\r` | **Tone Generator Test**<br>- Emits test tone on GPIO/buzzer for diagnostics<br>- `<enable>`: `0`=off, `1`=on<br>- `<freq>`: frequency in Hz (e.g. 1000)<br>- `<dur>`: duration in ms (e.g. 500)<br>**Response:** `+TTONE: <enable>,<freq>,<dur>` then `OK` [web:4] |
| **AT+TOFF**  | `AT+TOFF?\r`<br>`AT+TOFF\r`<br>`AT+TOFF=200\r` | **Tone Off / Stop**<br>- Stops ongoing tone OR sets off-interval for tone pattern<br>- No params: immediate stop<br>- `<ms>`: off duration between tones (ms)<br>**Response:** `+TOFF: <ms>` then `OK` [web:4] |


---

## Contributions

Feel free to contribute by improving the existing code or adding new features. Submit issues or pull requests for suggestions.

---

## License

This project is licensed under the MIT License. See the LICENSE file for details.
