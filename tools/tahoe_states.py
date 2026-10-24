"""Original rounded state families; keep percentages and badges distinguishable."""
import re


def state_for(name):
    name = name.replace('-symbolic','')
    levels = {'full':100,'good':80,'medium':50,'low':20,'caution':10,'critical':5,'empty':0}
    peripheral = re.fullmatch(r'(?:gpm-|xfpm-)(keyboard|mouse|phone)-(\d{1,3})',name)
    if not peripheral: peripheral = re.fullmatch(r'(keyboard|mouse|phone)-battery-(full|good|medium|low)',name)
    if peripheral:
        device,level=peripheral.groups();value=int(level) if level.isdigit() else levels[level]
        if value<=100:return f'sequoia_peripheral_{device}_{value}'
    if name in {'input-keyboard-battery','input-mouse-battery','phone-battery'}:
        return 'sequoia_peripheral_'+('keyboard' if 'keyboard' in name else 'mouse' if 'mouse' in name else 'phone')+'_100'
    mobile = re.fullmatch(r'network-mobile-(\d{1,3})(?:-(5g|edge|gprs|hsdpa|hspa|hsupa|lte|umts))?(-locked)?',name)
    if mobile:
        level,technology,locked=mobile.groups()
        if int(level)<=100:return f'sequoia_cellular_{int(level)}_{technology or "none"}_{"locked" if locked else "normal"}'
    cellular = re.fullmatch(r'network-cellular-(secure-)?signal-(none|weak|low|ok|good|excellent)',name)
    if cellular:
        secure,level=cellular.groups()
        return f'sequoia_cellular_{dict(none=0,weak=20,low=40,ok=60,good=80,excellent=100)[level]}_none_{"locked" if secure else "normal"}'
    gsm = re.fullmatch(r'gsm-3g-(none|low|medium|high|full)(-secure)?',name)
    if gsm:
        level,secure=gsm.groups()
        return f'sequoia_cellular_{dict(none=0,low=20,medium=50,high=80,full=100)[level]}_umts_{"locked" if secure else "normal"}'
    device = re.fullmatch(r'(smartphone|tablet|laptop)-?(connected|disconnected|trusted)',name)
    if device:return 'sequoia_connection_'+'_'.join(device.groups())
    battery = re.fullmatch(r'battery-(?:level-)?(\d{1,3}|full|good|medium|low|caution|critical|empty|missing)(?:-(charging|charged))?', name)
    if battery:
        level,state = battery.groups()
        if level == 'missing': return 'sequoia_battery_missing'
        value = int(level) if level.isdigit() else levels[level]
        if value > 100: return None
        return f'sequoia_battery_{value}_{state or "normal"}'
    wireless = re.fullmatch(r'(?:network-wireless-(?:connected-)?|nm-signal-)(\d{1,3})(?:-(locked|limited|secure))?', name)
    if wireless:
        level,badge=wireless.groups()
        if int(level)>100: return None
        return f'sequoia_wifi_{int(level)}_{"locked" if badge=="secure" else badge or "normal"}'
    signal = re.fullmatch(r'network-wireless-(secure-)?signal-(none|weak|low|ok|good|excellent)(-secure)?',name)
    if signal:
        secure,level,end=signal.groups()
        return f'sequoia_wifi_{dict(none=0,weak=20,low=40,ok=60,good=80,excellent=100)[level]}_{"locked" if secure or end else "normal"}'
    mic = re.fullmatch(r'(?:microphone-sensitivity|audio-input-microphone)-(low|medium|high)',name)
    if mic: return 'sequoia_mic_'+mic.group(1)
    return None


def register(groups, root):
    # Include mapped names even when rebuilding from upstream, where aliases
    # introduced by an earlier Sequoia release do not exist yet.
    names = {name.replace('-symbolic','') for group in groups.values()
             for aliases in group.values() for name in aliases.split() if state_for(name)}
    for theme in root.glob('SequoiaTahoe*'):
        if not theme.is_dir(): continue
        for context in ('actions','status','devices'):
            for path in (theme/context).rglob('*.svg'):
                if state_for(path.stem): names.add(path.stem.replace('-symbolic',''))
    names.update(f'battery-{level:03d}{suffix}' for level in range(0,101,10) for suffix in ('','-charging'))
    for context in ('actions','status','devices'):
        flat = {n:s for s,ns in groups[context].items() for n in ns.split()}
        for name in names:
            if context == 'status' or name in flat: flat[name]=state_for(name)
        grouped = {}
        for name,symbol in flat.items():grouped.setdefault(symbol,[]).append(name)
        groups[context]={s:' '.join(sorted(ns)) for s,ns in grouped.items()}


def geometry(symbol):
    stroke = 'fill="none" stroke="currentColor" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round"'
    thin = stroke.replace('1.65','1.3')
    if symbol.startswith('sequoia_soft_'):
        kind=symbol.removeprefix('sequoia_soft_')
        paper='<rect x="4" y="2.5" width="16" height="19" rx="3.3"/>'
        shapes={
            'clipboard':'<path d="M8 4.5H7a3 3 0 0 0-3 3v11a3 3 0 0 0 3 3h10a3 3 0 0 0 3-3v-11a3 3 0 0 0-3-3h-1"/><rect x="8" y="2" width="8" height="5" rx="2"/>',
            'copy':'<rect x="8" y="2.5" width="12" height="15" rx="3"/><path d="M5.5 7H5a2 2 0 0 0-2 2v10a2.5 2.5 0 0 0 2.5 2.5H15"/>',
            'filecopy':'<path d="M14 2.5h-3a3 3 0 0 0-3 3v9a3 3 0 0 0 3 3h6a3 3 0 0 0 3-3v-6a2 2 0 0 0-.6-1.4l-4-4a2 2 0 0 0-1.4-.6ZM14 3v4a1.5 1.5 0 0 0 1.5 1.5h4M5 7a2 2 0 0 0-2 2v10a2.5 2.5 0 0 0 2.5 2.5H15"/>',
            'newdocument':paper+'<path d="M8 12h8m-4-4v8"/>',
            'document':paper+'<path d="M8 7.5h8M8 12h8m-8 4.5h5"/>',
            'save':'<path d="M6 3h9.8a2 2 0 0 1 1.4.6l3.2 3.2a2 2 0 0 1 .6 1.4V18a3 3 0 0 1-3 3H6a3 3 0 0 1-3-3V6a3 3 0 0 1 3-3Z"/><path d="M7 3v4a1.5 1.5 0 0 0 1.5 1.5h5A1.5 1.5 0 0 0 15 7V3"/><circle cx="12" cy="15" r="3"/>',
            'window':'<rect x="3" y="3" width="18" height="18" rx="3.6"/>',
        }
        return f'<g {stroke.replace("1.65","1.9")}>{shapes[kind]}</g>'
    def badge(kind):
        if kind=='locked':return f'<g {thin}><rect x="17.5" y="17" width="5" height="4.5" rx=".8"/><path d="M18.5 17v-1.5a1.5 1.5 0 0 1 3 0V17"/></g>'
        if kind=='connected':return f'<path class="ColorScheme-PositiveText" {stroke} d="m17 19 1.8 1.8 3.5-4"/>'
        if kind=='disconnected':return f'<path class="ColorScheme-NegativeText" {stroke} d="m18 17 4 4m0-4-4 4"/>'
        if kind=='limited':return f'<g class="ColorScheme-NeutralText" {thin}><circle cx="20" cy="19" r="2.7"/><path d="M20 17.5v1.4m0 1.4v.1"/></g>'
        return ''
    if symbol.startswith('sequoia_peripheral_'):
        device,level=symbol.removeprefix('sequoia_peripheral_').split('_');level=int(level)
        if device=='phone':body='<rect x="2" y="3.5" width="8.5" height="17" rx="1.8"/><path d="M5 6h2.5m-2 12h1.5"/>'
        elif device=='mouse':body='<rect x="2" y="5" width="8.5" height="15" rx="4.2"/><path d="M6.2 8v3"/>'
        else:
            body='<rect x="1.5" y="6.5" width="10.5" height="12" rx="1.5"/><path d="M4 9.5h.1m2.6 0h.1m2.6 0h.1M4 12.5h.1m2.6 0h.1m2.6 0h.1M4 15.5h5.4"/>'
        body=f'<g {thin}>{body}<rect x="15" y="8" width="6.5" height="12" rx="1.2"/><path d="M17.2 6h2"/></g>'
        if level:body+=f'<rect x="16.5" y="{18.5-9*level/100:.3f}" width="3.5" height="{9*level/100:.3f}" rx=".4" fill="currentColor"/>'
        return body
    if symbol.startswith('sequoia_cellular_'):
        level,technology,state=symbol.removeprefix('sequoia_cellular_').split('_');level=int(level)
        result=''
        for i,height in enumerate((4,7,10,13,16)):
            opacity=.2+.8*min(max((level-i*20)/20,0),1)
            result+=f'<rect x="{2+i*2.8:.2f}" y="{21-height}" width="1.8" height="{height}" rx=".9" fill="currentColor" opacity="{opacity:.3f}"/>'
        # Vector labels preserve mobile technologies without font dependence.
        letters={'G':'M2.5 .5H.5v4h2V2.7H1.6','E':'M2.5 .5H.5v4h2M.5 2.5h1.6','H':'M.5 .5v4m2-4v4M.5 2.5h2','L':'M.5 .5v4h2','T':'M0 .5h3M1.5 .5v4','3':'M.5 .5h2v4h-2m1-2h1','5':'M2.5 .5h-2v2h2v2h-2','+':'M0 2.5h3M1.5 1v3','d':'M1.5 .5v4M.3 3.3l1.2 1.2 1.2-1.2','u':'M1.5 4.5v-4M.3 1.7 1.5 .5l1.2 1.2'}
        label={'none':'','5g':'5G','edge':'E','gprs':'G','hsdpa':'Hd','hspa':'H+','hsupa':'Hu','lte':'LTE','umts':'3G'}[technology]
        for i,char in enumerate(label):result+=f'<path fill="none" stroke="currentColor" stroke-width=".8" stroke-linecap="round" stroke-linejoin="round" transform="translate({2+i*3.5} 1.5)" d="{letters[char]}"/>'
        return result+badge(state)
    if symbol.startswith('sequoia_connection_'):
        device,state=symbol.removeprefix('sequoia_connection_').split('_')
        if device=='smartphone':body='<rect x="4" y="2.5" width="10" height="19" rx="2"/><path d="M7.5 5h3M8 19h2"/>'
        elif device=='tablet':body='<rect x="2" y="3" width="13" height="18" rx="2"/><path d="M7.5 18.5h2"/>'
        elif device=='laptop':body='<rect x="2.5" y="4" width="13" height="12" rx="1.4"/><path d="M2.5 16 1 19h14"/>'
        elif device=='wired':body='<path d="M3 3.5h11v4h2v6H1v-6h2zM4.5 7.5v2m3-2v2m3-2v2M8.5 14v5m-5 0h10"/>'
        elif device=='bluetooth':body='<path d="M8 2.5v19l7-5.5L3 7m0 10L15 8 8 2.5"/>'
        else:raise ValueError(device)
        return f'<g {stroke}>{body}</g>'+badge('locked' if state=='trusted' else state)
    if symbol=='sequoia_ups':return f'<g {stroke}><rect x="5" y="2.5" width="14" height="19" rx="2.5"/><path d="M8 6.5h8M9 18h6m-3-10v2"/><circle cx="12" cy="12" r="3"/></g>'
    if symbol=='sequoia_projector':return f'<g {stroke}><rect x="2" y="5" width="20" height="13" rx="2.5"/><circle cx="16" cy="11.5" r="3.5"/><path d="M5 9v5m3-5v5M5 18v2m14-2v2"/></g>'
    if symbol=='sequoia_keyboard_light':return f'<g {thin}><rect x="2" y="9" width="17" height="11" rx="1.5"/><path d="M5 12h.1m3 0h.1m3 0h.1m3 0h.1M5 15h.1m3 0h.1m3 0h.1m3 0h.1M6 17.5h9M17 1.5v1m4 1-1 .7m2.5 2.8h-1M12.5 3.5l1 .7"/><circle cx="17" cy="6.5" r="2"/></g>'
    if symbol=='sequoia_wireless_bluetooth':return f'<g {stroke}><path d="M2 8.5q6-6 12 0M4.5 12q3.5-3.5 7 0M7 15.5q1-.8 2 0M18 2v14l4-4-8-6m0 6 8-6-4-4"/><circle cx="8" cy="19" r=".5"/></g>'
    if symbol=='sequoia_invert_selection':return f'<g {stroke}><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M12 3v18M3 12h18"/></g><path d="M4.7 4.7h5.6v5.6H4.7zm9 9h5.6v5.6h-5.6z"/>'
    if symbol=='sequoia_object_group':return f'<g {stroke}><rect x="2.5" y="2.5" width="19" height="19" rx="2.5"/><rect x="6" y="6" width="5" height="5" rx=".8"/><circle cx="16" cy="8.5" r="2.5"/><path d="m5.5 18 3-5 3 5zm8-4h5v4h-5z"/></g>'
    if symbol.startswith('sequoia_delete_'):
        direction=symbol.removeprefix('sequoia_delete_')
        inner='M3 8h15M3 13h15' if direction=='row' else 'M8 3v15m5-15v15'
        return f'<g {stroke}><rect x="3" y="3" width="15" height="15" rx="1.5"/><path d="{inner}"/></g><circle cx="19" cy="19" r="3.5" class="ColorScheme-NegativeText" fill="currentColor"/><path d="M17.5 19h3" fill="none" stroke="white" stroke-width="1.4" stroke-linecap="round"/>'
    if symbol.startswith('sequoia_battery_'):
        detail=symbol.removeprefix('sequoia_battery_')
        missing=detail=='missing'
        level,state=(0,'missing') if missing else (int(detail.split('_')[0]),detail.split('_')[1])
        frame=f'<g {stroke}><rect x="2.2" y="7" width="15" height="10" rx="2"/><path d="M19 10v4"/></g>'
        if missing:
            return frame+f'<path {stroke} d="m6.5 9.5 5 5m0-5-5 5"/>'
        if level:
            frame += f'<rect x="4.4" y="9.2" width="{10.6*level/100:.3f}" height="5.6" rx="{min(.8,5.3*level/100):.3f}" fill="currentColor"/>'
        if state=='charging':
            # The bolt stays outside the fill, preserving every charge level.
            frame=frame.replace('<path d="M19 10v4"/>','')
            frame+=f'<path {stroke.replace('1.65','1.4')} d="m22 6-3.4 6h3.5l-3.2 6"/>'
        elif state=='charged':
            frame=frame.replace('<path d="M19 10v4"/>','')
            frame+=f'<path {stroke} d="m19 12 1.2 1.5 2-3"/>'
        return frame
    if symbol.startswith('sequoia_wifi_'):
        level,badge=symbol.removeprefix('sequoia_wifi_').split('_');level=int(level)
        paths=['M9 14.5q3-2.8 6 0','M6 11.5q6-5.5 12 0','M3 8.5q9-8.2 18 0']
        result=f'<circle cx="12" cy="18.1" r="1.15" fill="currentColor" opacity="{.22+.78*min(level/25,1):.3f}"/>'
        for threshold,path in zip((25,50,75),paths):
            opacity=.22+.78*min(max((level-threshold)/25,0),1)
            result+=f'<path {stroke} d="{path}" opacity="{opacity:.3f}"/>'
        if badge=='locked':
            result+=f'<g {stroke.replace('1.65','1.3')}><rect x="18" y="17.5" width="4.6" height="4" rx=".8"/><path d="M19 17.5V16a1.3 1.3 0 0 1 2.6 0v1.5"/></g>'
        elif badge=='limited':
            result+=f'<g class="ColorScheme-NeutralText" {stroke.replace('1.65','1.3')}><circle cx="20" cy="19" r="2.7"/><path d="M20 17.5v1.7"/></g><circle class="ColorScheme-NeutralText" cx="20" cy="20.4" r=".6" fill="currentColor"/>'
        return result
    if symbol.startswith('sequoia_mic_'):
        level={'low':1,'medium':2,'high':3}[symbol.removeprefix('sequoia_mic_')]
        result=f'<g {stroke}><rect x="5.5" y="3" width="5" height="10" rx="2.5"/><path d="M3 10v1a5 5 0 0 0 10 0v-1M8 16v4m-2.5 0h5"/></g>'
        for index,height in enumerate((3,6,9)):
            result+=f'<path {stroke} d="M{16+index*2.5} 16v-{height}" opacity="{1 if index<level else .22}"/>'
        return result
    raise ValueError(symbol)
