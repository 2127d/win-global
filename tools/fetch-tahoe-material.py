#!/usr/bin/env python3
"""Fetch only reviewed SVGs from the pinned official Material Symbols revision."""
import concurrent.futures
import hashlib
import json
import urllib.request
from tahoe_material import SOURCE, REVISION, symbols, source_variant

base = f'https://raw.githubusercontent.com/google/material-design-icons/{REVISION}'
previous = json.loads((SOURCE/'sources.json').read_text()) if (SOURCE/'sources.json').exists() else {'sha256': {}}

def fetch(symbol_and_size):
    symbol,size = symbol_and_size
    base_symbol,variant = source_variant(symbol)
    name = f'{symbol}-{size}.svg'
    remote = f'symbols/web/{base_symbol}/materialsymbolsrounded/{base_symbol}{variant}_{size}px.svg'
    target = SOURCE/name
    digest = hashlib.sha256(target.read_bytes()).hexdigest() if target.exists() else None
    if digest != previous['sha256'].get(name) or not target.exists():
        with urllib.request.urlopen(base+'/'+remote, timeout=40) as response: data=response.read()
        if not data.lstrip().startswith(b'<svg'): raise RuntimeError(remote)
        target.write_bytes(data)
        digest = hashlib.sha256(data).hexdigest()
    return name,digest,remote

jobs=[(symbol,size) for symbol in symbols() if not symbol.startswith('sequoia_') for size in (20,24)]
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
    results=list(pool.map(fetch,jobs))
hashes={name:digest for name,digest,_ in results}
hashes['LICENSE']=hashlib.sha256((SOURCE/'LICENSE').read_bytes()).hexdigest()
(SOURCE/'sources.json').write_text(json.dumps({'revision':REVISION,'source':base,'sha256':hashes,
    'paths':{name:remote for name,_,remote in results}},indent=2)+'\n')
print(f'Verified {len(results)} pinned SVG sources')
