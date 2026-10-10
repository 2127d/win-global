#!/usr/bin/env python3
"""Verify upstream uBlock semantics through the same local worker protocol."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--node', type=Path, default=ROOT / '.staging/falkon-native-build/bin/node')
args = parser.parse_args()
rules = '\n'.join([
    '||ads.example^', '@@||ads.example/allowed.js$script',
    '||ads.example/priority.js$script,important', '@@||ads.example/priority.js$script',
    '||third.test^$third-party', '||images.test^$image',
    '||method.test^$method=post', '||example.test^$removeparam=track',
    '||scoped.test^$domain=allowed.test',
])
cases = [
    ('https://ads.example/a.js', 'https://example.test/', 'script', 'GET', 1),
    ('https://ads.example/allowed.js', 'https://example.test/', 'script', 'GET', 2),
    ('https://ads.example/priority.js', 'https://example.test/', 'script', 'GET', 1),
    ('https://third.test/a', 'https://example.test/', 'script', 'GET', 1),
    ('https://third.test/a', 'https://sub.third.test/', 'script', 'GET', 0),
    ('https://images.test/a', 'https://example.test/', 'image', 'GET', 1),
    ('https://images.test/a', 'https://example.test/', 'script', 'GET', 0),
    ('https://method.test/a', 'https://example.test/', 'xmlhttprequest', 'POST', 1),
    ('https://method.test/a', 'https://example.test/', 'xmlhttprequest', 'GET', 0),
    ('https://scoped.test/a', 'https://allowed.test/', 'script', 'GET', 1),
    ('https://scoped.test/a', 'https://other.test/', 'script', 'GET', 0),
]
requests = [{'id': 1, 'op': 'lists', 'lists': [{'name': 'Fixtures', 'raw': rules}]}]
for url, origin, kind, method, _ in cases:
    requests.append({'id': len(requests) + 1, 'op': 'match', 'url': url,
                     'originURL': origin, 'type': kind, 'method': method})
requests.append({'id': len(requests) + 1, 'op': 'match',
                 'url': 'https://example.test/a?track=x&keep=1',
                 'originURL': 'https://example.test/', 'type': 'main_frame'})
result = subprocess.run([str(args.node), str(ROOT / 'apps/falkon/resources/ublock/worker.mjs')],
                        input=''.join(json.dumps(r) + '\n' for r in requests),
                        text=True, capture_output=True, timeout=20, check=True)
responses = [json.loads(line) for line in result.stdout.splitlines()]
assert len(responses) == len(requests), result.stdout
assert responses[0]['ready']
for response, case in zip(responses[1:], cases):
    assert response['decision'] == case[-1], (case, response)
assert responses[-1]['redirect'] == 'https://example.test/a?keep=1', responses[-1]
print('PASS: blocking, exceptions, important precedence, third-party/domain scope, resource types, HTTP methods and query removal')
