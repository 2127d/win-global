#!/usr/bin/env python3
"""Small local Marionette client for theme validation in a disposable profile.

Start Firefox with --marionette --remote-allow-system-access and marionette.port=2829.
Never enable remote system access in your everyday Firefox profile.
"""
import json
import socket


class Browser:
    def __init__(self, port=2829):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout=30)
        self.sequence = 0
        self.receive()
        self.command('WebDriver:NewSession', {'capabilities': {'alwaysMatch': {'acceptInsecureCerts': False}}})
        self.command('Marionette:SetContext', {'value': 'chrome'})

    def receive(self):
        length = b''
        while (char := self.socket.recv(1)) != b':':
            if not char:
                raise ConnectionError('Firefox closed the connection')
            length += char
        data = b''
        while len(data) < int(length):
            data += self.socket.recv(int(length) - len(data))
        return json.loads(data)

    def command(self, name, parameters=None):
        self.sequence += 1
        data = json.dumps([0, self.sequence, name, parameters or {}]).encode()
        self.socket.sendall(str(len(data)).encode() + b':' + data)
        response = self.receive()
        if response[2]:
            raise RuntimeError(response[2])
        return response[3]

    def js(self, script):
        return self.command('WebDriver:ExecuteScript', {'script': script, 'args': [], 'newSandbox': False})['value']

    def close(self):
        self.command('WebDriver:DeleteSession')
        self.socket.close()


if __name__ == '__main__':
    browser = Browser()
    try:
        print(json.dumps(browser.js('''
          return [...document.querySelectorAll('#nav-bar, #sidebar-box, #sidebar-container, #tabbrowser-tabbox, #urlbar-background')].map(el => {
            const s = getComputedStyle(el), r = el.getBoundingClientRect();
            return {id: el.id, background: s.backgroundColor, radius: s.borderRadius,
              margin: s.margin, width: r.width, height: r.height, shadow: s.boxShadow};
          });
        '''), indent=2))
    finally:
        browser.close()
