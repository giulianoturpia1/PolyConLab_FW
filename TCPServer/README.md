Simple TCP client (Python)

This small client connects to a TCP server and lets you send text lines from the terminal.

Defaults:

- Host: 192.168.1.1
- Port: 80

Usage:

python client.py
python client.py --host 192.168.1.1 --port 80

Behavior:

- Type a line and press Enter to send it to the server.
- Incoming data from the server is printed to the terminal.
- Press Ctrl+C to exit.

Notes:

- Ensure the PC running this script is on the same network as the server and can reach 192.168.1.2.
- If your server listens on a different port, pass it with `--port`.
