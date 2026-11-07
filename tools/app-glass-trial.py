#!/usr/bin/env python3
"""Apply or restore the reversible liquid-glass treatment on transparent apps."""
import argparse
from app_glass import start,restore
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('action',choices=['start','restore','enable-persistent'])
p.add_argument('--logout',action='store_true',help='Restore settings without accessing the closing compositor')
a=p.parse_args()
if a.action=='enable-persistent':
    start()
    from glass_persistence import enable
    from app_glass import refresh_surfaces
    enable()
    refresh_surfaces()
    print('Liquid glass is enabled for future logins. The restore control disables persistence too.')
elif a.action=='start':start()
else:restore(a.logout)
