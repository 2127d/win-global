#!/usr/bin/env python3
"""Inventory converted names and retained small monochrome artwork."""
import json
from pathlib import Path
from tahoe_material import SOURCE, outline_asset

root=SOURCE.parent
manifest=json.loads((root/'material-icons.json').read_text())
generated={root/p for p in manifest['generated']}
theme=root/'SequoiaTahoe-light'
report={}
for context in ('actions','status','devices','categories','places','emblems','preferences','apps','mimes'):
    converted=set(); retained=set(); other=set()
    for path in (theme/context).rglob('*.svg'):
        if not any(p in {'16','22','24','32','symbolic'} for p in path.relative_to(theme/context).parts[:-1]): continue
        if context in {'apps','mimes'} and 'symbolic' not in path.parts: continue
        name=path.stem
        if path in generated or path.resolve() in generated: converted.add(name)
        elif outline_asset(path): retained.add(name)
        else: other.add(name)
    report[context]={'converted_names':sorted(converted),
                     'retained_monochrome_names':sorted(retained-converted),
                     'retained_other_art_names':sorted(other-converted)}
data={'scope':'Small UI artwork (16/22/24/32 and symbolic); apps and MIME artwork only in symbolic directories.',
      'retention_policy':'Unmapped or ambiguous artwork is retained. Specialized notation, direction/state badges and brand identities require an explicit equivalent. Large full-color artwork is excluded.',
      'designs_and_state_variants':manifest['glyphs'],
      'generated_svg_files':len(manifest['generated']),
      'contexts':report}
(SOURCE/'coverage.json').write_text(json.dumps(data,indent=2)+'\n')
for context,entry in report.items():print(context, *(f'{k}: {len(v)}' for k,v in entry.items()))
