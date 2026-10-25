#!/usr/bin/env python3
"""Exercise the Sequoia Weather UI against deterministic, isolated forecast caches."""
import argparse
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def fixture(base, lat, lon, name, icon, location_index=None):
    offset = 20 if name == 'Monterey' else 0
    today=datetime.now(timezone.utc).replace(hour=0,minute=0,second=0,microsecond=0)
    days=[]
    for index in range(7):
        date=today+timedelta(days=index)
        hourly=[]
        for hour in range(24):
            hourly.append(dict(date=(date+timedelta(hours=hour)).isoformat(),temperature=offset+14+index+hour/8,
                weatherIcon=icon,neutralWeatherIcon=icon,weatherDescription='Rain' if 'showers' in icon else 'Clear',
                humidity=72,pressure=1013,uvIndex=2,precipitationAmount=0.3,windSpeed=12,windDirectionDegree=220,
                dewPointTemperature=8,fog=0))
        days.append(dict(date=date.strftime('%Y-%m-%d'),hourly=hourly,maxTemp=offset+18+index,minTemp=offset+9+index,
            weatherIcon=icon,weatherDescription=hourly[0]['weatherDescription'],humidity=72,pressure=1013,
            uvIndex=2+index*0.4,precipitation=2.4))
    # A zero-range day at the scale's upper endpoint exercises the visible marker.
    days[-1]['minTemp'] = days[-1]['maxTemp']
    target=base/'cache/kweather/cache'/f'{lat:.2f}'/f'{lon:.2f}'/'cache.json'
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(dict(createdTime=datetime.now(timezone.utc).isoformat(),lat=lat,lon=lon,timezone='UTC',day=days)))
    if location_index is None:
        location_index=0 if name=='Tooele' else 1
    return f'[WeatherLocations][{name}]\nindex={location_index}\nlatitude={lat}\nlongitude={lon}\nlocationName={name}\ntimezone=UTC\n'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'.staging/kweather-build/bin/kweather')
    parser.add_argument('--mode',choices=('light','dark'),default='dark')
    parser.add_argument('--material',choices=('solid','transparent'),default='solid')
    parser.add_argument('--icon-theme',default='breeze',help='Icon theme used only by the isolated test session')
    parser.add_argument('--compositing',action='store_true',help='Stress material switching and check modal scrim pixels at the client edges')
    parser.add_argument('--empty',action='store_true')
    parser.add_argument('--welcome',action='store_true',help='Test first launch with no saved locations')
    parser.add_argument('--compact',action='store_true',help='Check all auxiliary pages at the smallest supported window size')
    parser.add_argument('--choices',action='store_true',help='Check inline Settings choices, keyboard access and saved values')
    parser.add_argument('--rendered',action='store_true',help='Use the live Wayland renderer to check shader effects with isolated test data')
    args=parser.parse_args()
    helper=ROOT/'.staging/verify-kweather.so'; source=ROOT/'apps/kweather/tests/verify.cpp'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Widgets','Qt6Quick','Qt6Test','Qt6Network'],text=True))
    if not helper.exists() or source.stat().st_mtime>helper.stat().st_mtime:
        subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(source),'-o',str(helper),*flags],check=True)
    output=ROOT/(f'preview/kweather-{args.mode}-{args.material}'+('-welcome' if args.welcome else '-empty' if args.empty else '-compact' if args.compact else '')+('-choices' if args.choices else '')+('-rendered' if args.rendered else '')+('-compositing' if args.compositing else '')+('-icons-'+args.icon_theme if args.icon_theme!='breeze' else ''));output.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-weather-test-',ignore_cleanup_errors=True) as temp:
        base=Path(temp);env=dict(os.environ)
        for kind in ('CONFIG','DATA','CACHE','STATE','RUNTIME'):
            path=base/kind.lower();path.mkdir(mode=0o700)
            env[f'XDG_{kind}_HOME' if kind!='RUNTIME' else 'XDG_RUNTIME_DIR']=str(path)
        (base/'config/kweather').mkdir()
        config='[General]\nfirstStartup=false\nforecastStyle=Dynamic\ntemperatureUnits=Celsius\n\n'
        config+=fixture(base,40.53,-112.30,'Tooele','weather-showers')+'\n'+fixture(base,36.60,-121.89,'Monterey','weather-clear-night')
        if args.compact:
            for index in range(2,8):
                config+='\n'+fixture(base,40+index,-110,'Test location '+str(index),'weather-clear',index)
        if args.welcome:
            config='[General]\nfirstStartup=true\nforecastStyle=Dynamic\n'
        (base/'config/kweather/kweatherrc').write_text(config)
        if args.empty:
            import shutil
            shutil.rmtree(base/'cache/kweather/cache')
        (base/'config/kdeglobals').write_text((ROOT/f'themes/color-schemes/SequoiaEleven{args.mode.title()}{args.material.title()}.colors').read_text()+'\n[Icons]\nTheme='+args.icon_theme+'\n')
        env.update(QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='kde',QT_STYLE_OVERRIDE='Fusion',QT_QUICK_BACKEND='software',
            QT_FORCE_STDERR_LOGGING='1',XDG_DATA_DIRS=str(Path.home()/'.local/share')+':/usr/local/share:/usr/share',WEATHER_CAPTURE=str(output),WEATHER_MATERIAL=args.material,LC_ALL='C.UTF-8')
        env.pop('LD_PRELOAD',None)
        if args.rendered:
            env.update(QT_QPA_PLATFORM='wayland',QT_QUICK_BACKEND='rhi',QSG_RHI_BACKEND='opengl',
                XDG_RUNTIME_DIR=os.environ['XDG_RUNTIME_DIR'])
        env['WEATHER_COMPOSITING']='1' if args.compositing else ''
        env['WEATHER_RENDERED']='1' if args.rendered else ''
        env['WEATHER_EMPTY']='1' if args.empty else ''
        env['WEATHER_WELCOME']='1' if args.welcome else ''
        env['WEATHER_COMPACT']='1' if args.compact else ''
        env['WEATHER_CHOICES']='1' if args.choices else ''
        color_line=(base/'config/kdeglobals').read_text().split('[Colors:Window]',1)[1].split('BackgroundNormal=',1)[1].splitlines()[0]
        channels=color_line.split(',');env['WEATHER_ALPHA']=channels[3] if len(channels)==4 else '255'
        with (output/'verification.log').open('w') as log_file:
            result=subprocess.run(['dbus-run-session','--','env','LD_PRELOAD='+str(helper),str(args.binary.resolve())],env=env,stdout=log_file,stderr=log_file,timeout=35)
        log=(output/'verification.log').read_text()
        print('Exit:',result.returncode);print(log[-7000:])
        assert result.returncode==0 and 'PASS:' in log and 'FAIL:' not in log
        assert not any(s in log for s in ('ReferenceError:', 'TypeError:', 'Binding loop detected', 'Cannot assign', 'Unable to assign', 'is not a type', 'Unknown Weather action icon:', 'Cannot open:', 'Error decoding:'))

if __name__=='__main__':main()
