#!/usr/bin/env python3
"""Compare scene-inspector JSON with its recorded SHA256.

Usage: check_scene_fixture.py SCENE_JSON (or '-' to read stdin).
No game asset data is stored by this checker.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

EXPECTED_SHA256='543050bc5ef3a31a11130f80843a9ec2188b622be6c0fd00b2c10d164d502e05'

def canonical(value):
    if isinstance(value,dict):return {key:canonical(item) for key,item in value.items()}
    if isinstance(value,list):return [canonical(item) for item in value]
    if isinstance(value,float):
        if not math.isfinite(value):raise ValueError('Non-finite scene JSON number')
        if value.is_integer():return int(value)
    return value

def scene_digest(scene):
    # Ignore object-key ordering, whitespace and integer/float spelling while
    # retaining scene/array order and the complete exported numeric values.
    data=json.dumps(canonical(scene),sort_keys=True,separators=(',',':'),
        ensure_ascii=True,allow_nan=False).encode('utf-8')
    return hashlib.sha256(data).hexdigest()

def matches(scene,expected=EXPECTED_SHA256):
    return scene_digest(scene)==expected

def self_test():
    reference={'b':True,'a':[1,0,0.5]}
    expected=hashlib.sha256(b'{"a":[1,0,0.5],"b":true}').hexdigest()
    assert matches(reference,expected)
    assert matches(json.loads('{ "a": [1.0, -0.0, 0.50], "b": true }'),expected)
    assert not matches({'b':True,'a':[1,0,0.25]},expected)
    assert not matches({'b':True,'a':[0,1,0.5]},expected)
    assert not matches({'b':False,'a':[1,0,0.5]},expected)
    print('Scene fixture hash checks passed')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('scene',nargs='?',help='Scene JSON file, or - for stdin')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:self_test();return 0
    if not args.scene:parser.error('Supply scene-inspector JSON or --self-test')
    scene=json.load(sys.stdin) if args.scene=='-' else json.loads(Path(args.scene).read_text())
    if not matches(scene):
        print('Scene fixture SHA256 mismatch',file=sys.stderr);return 1
    print('Scene fixture SHA256 matches');return 0

if __name__=='__main__':raise SystemExit(main())
