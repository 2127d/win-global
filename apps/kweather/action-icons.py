#!/usr/bin/env python3
"""Bundle reviewed Sequoia control SVGs, independent of desktop theme lookup."""
import hashlib
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import tahoe_material as material

ALIASES={
    'settings-configure':'settings','configure':'settings','view-refresh':'refresh',
    'list-add':'add','find-location':'location_on','search':'search',
    'window-close-symbolic':'close','edit-clear-locationbar-ltr':'close','edit-clear-locationbar-rtl':'close',
    'go-down-symbolic':'keyboard_arrow_down','go-up-symbolic':'keyboard_arrow_up',
    'go-next-symbolic':'arrow_forward','go-previous-symbolic':'arrow_back',
    'arrow-left':'arrow_back','arrow-right':'arrow_forward',
    'dialog-ok-apply':'check','checkmark':'check','handle-sort':'more_vert',
    'delete':'delete','edit-delete':'delete','network-disconnect':'link_off',
    'globe':'public','globe-symbolic':'public','mail-sent-symbolic':'send',
    'tools-report-bug-symbolic':'bug_report','applications-development-symbolic':'code',
    'donate-symbolic':'favorite_filled','edit-copy-symbolic':'content_copy',
}

def build(output):
    output.mkdir(parents=True,exist_ok=True)
    qrc=ET.Element('RCC');resource=ET.SubElement(qrc,'qresource',prefix='/sequoia-weather')
    manifest={}
    for symbol in sorted(set(ALIASES.values())):
        path=output/(symbol+'.svg');data=material.svg(symbol,24,'#363636')
        path.write_text(data);manifest[path.name]=hashlib.sha256(data.encode()).hexdigest()
        ET.SubElement(resource,'file',alias='actions/'+path.name).text=str(path)
    js=output/'ActionIcons.js'
    js.write_text('.pragma library\n// Private control resources; do not use for app identities or forecast artwork.\nvar aliases = '+json.dumps(ALIASES,sort_keys=True)+';\n'
                  'function source(name) {\n    if (!name) return "";\n'
                  '    if (!aliases[name]) { console.warn("Unknown Weather action icon: " + name); return name; }\n'
                  '    return "qrc:/sequoia-weather/actions/" + aliases[name] + ".svg";\n}\n')
    license=output/'COPYING.MaterialSymbols';license.write_bytes((material.SOURCE/'LICENSE').read_bytes())
    metadata=output/'action-icons.json';metadata.write_text(json.dumps({'revision':material.REVISION,'aliases':ALIASES,'sha256':manifest},indent=2)+'\n')
    for path in (js,license,metadata):ET.SubElement(resource,'file',alias=path.name).text=str(path)
    ET.ElementTree(qrc).write(output/'weather-actions.qrc',encoding='unicode')

if __name__=='__main__':build(Path(sys.argv[1]).resolve())
