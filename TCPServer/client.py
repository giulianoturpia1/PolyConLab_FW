#!/usr/bin/env python3
"""
Simple TCP client for sending text lines to a server over Ethernet.
Default target: 192.168.1.1:80 (configurable with --host/--port)

Usage:
  python client.py               # connects to 192.168.1.2:80
  python client.py --host 192.168.1.1 --port 80

Type lines and press Enter to send. Incoming server data is printed.
Ctrl+C to quit.
"""

import socket
import threading
import argparse
import sys


def recv_loop(sock):
    try:
        while True:
            data = sock.recv(4096)
            if not data:
                print('\n[Disconnected from server]')
                break
            # Print received data without interfering with prompt
            try:
                text = data.decode(errors='replace')
            except Exception:
                text = str(data)
            # Ensure newline for prompt clarity
            sys.stdout.write('\n' + text)
            sys.stdout.write('> ')
            sys.stdout.flush()
    except Exception as e:
        print(f"\n[Receive thread error: {e}]")


def main():
    parser = argparse.ArgumentParser(description='Simple TCP client to send text lines to a server')
    parser.add_argument('--host', '-H', default='192.168.1.1', help='Server IP (default: 192.168.1.1)')
    parser.add_argument('--port', '-p', type=int, default=80, help='Server port (default: 80)')
    args = parser.parse_args()

    addr = (args.host, args.port)
    print(f"Connecting to {addr[0]}:{addr[1]}...")

    try:
        sock = socket.create_connection(addr, timeout=5)
    except Exception as e:
        print(f"Could not connect: {e}")
        return

    print("Connected. Type text and press Enter to send. Ctrl+C to exit.")

    receiver = threading.Thread(target=recv_loop, args=(sock,), daemon=True)
    receiver.start()

    try:
        while True:
            try:
                line = input('> ')
            except EOFError:
                break
            # send line with newline so server can parse lines
            try:
                sock.sendall((line + '\n').encode())
            except BrokenPipeError:
                print('\n[Connection closed by server]')
                break
            except Exception as e:
                print(f"Send error: {e}")
                break
    except KeyboardInterrupt:
        print('\nInterrupted by user')
    finally:
        try:
            sock.shutdown(socket.SHUT_RDWR)
        except Exception:
            pass
        sock.close()
        print('Socket closed.')


if __name__ == '__main__':
    main()
