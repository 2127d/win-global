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

def fixture(base, lat, lon, name, icon):
    today=datetime.now(timezone.utc).replace(hour=0,minute=0,second=0,microsecond=0)
    days=[]
    for index in range(7):
        date=today+timedelta(days=index)
        hourly=[]
        for hour in range(24):
            hourly.append(dict(date=(date+timedelta(hours=hour)).isoformat(),temperature=14+index+hour/8,
                weatherIcon=icon,neutralWeatherIcon=icon,weatherDescription='Rain' if 'showers' in icon else 'Clear',
                humidity=72,pressure=1013,uvIndex=2,precipitationAmount=0.3,windSpeed=12,windDirectionDegree=220,
                dewPointTemperature=8,fog=0))
        days.append(dict(date=date.strftime('%Y-%m-%d'),hourly=hourly,maxTemp=18+index,minTemp=9+index,
            weatherIcon=icon,weatherDescription=hourly[0]['weatherDescription'],humidity=72,pressure=1013,
            uvIndex=2+index*0.4,precipitation=2.4))
    target=base/'cache/kweather/cache'/f'{lat:.2f}'/f'{lon:.2f}'/'cache.json'
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(dict(createdTime=datetime.now(timezone.utc).isoformat(),lat=lat,lon=lon,timezone='UTC',day=days)))
    return f'[WeatherLocations][{name}]\nindex={0 if name=="Tooele" else 1}\nlatitude={lat}\nlongitude={lon}\nlocationName={name}\ntimezone=UTC\n'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'.staging/kweather-build/bin/kweather')
    parser.add_argument('--mode',choices=('light','dark'),default='dark')
    parser.add_argument('--material',choices=('solid','transparent'),default='solid')
    parser.add_argument('--empty',action='store_true')
    args=parser.parse_args()
    helper=ROOT/'.staging/verify-kweather.so'; source=ROOT/'apps/kweather/tests/verify.cpp'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Widgets','Qt6Quick','Qt6Test','Qt6Network'],text=True))
    if not helper.exists() or source.stat().st_mtime>helper.stat().st_mtime:
        subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(source),'-o',str(helper),*flags],check=True)
    output=ROOT/(f'preview/kweather-{args.mode}-{args.material}'+('-empty' if args.empty else ''));output.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-weather-test-',ignore_cleanup_errors=True) as temp:
        base=Path(temp);env=dict(os.environ)
        for kind in ('CONFIG','DATA','CACHE','STATE','RUNTIME'):
            path=base/kind.lower();path.mkdir(mode=0o700)
            env[f'XDG_{kind}_HOME' if kind!='RUNTIME' else 'XDG_RUNTIME_DIR']=str(path)
        (base/'config/kweather').mkdir()
        config='[General]\nfirstStartup=false\nforecastStyle=Dynamic\ntemperatureUnits=Celsius\n\n'
        config+=fixture(base,40.53,-112.30,'Tooele','weather-showers')+'\n'+fixture(base,36.60,-121.89,'Monterey','weather-clear-night')
        (base/'config/kweather/kweatherrc').write_text(config)
        if args.empty:
            import shutil
            shutil.rmtree(base/'cache/kweather/cache')
        (base/'config/kdeglobals').write_text((ROOT/f'themes/color-schemes/SequoiaEleven{args.mode.title()}{args.material.title()}.colors').read_text()+'\n[Icons]\nTheme=breeze\n')
        env.update(QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='kde',QT_STYLE_OVERRIDE='Fusion',QT_QUICK_BACKEND='software',
            QT_FORCE_STDERR_LOGGING='1',XDG_DATA_DIRS=str(Path.home()/'.local/share')+':/usr/local/share:/usr/share',WEATHER_CAPTURE=str(output),WEATHER_MATERIAL=args.material,LC_ALL='C.UTF-8')
        env.pop('LD_PRELOAD',None)
        env['WEATHER_EMPTY']='1' if args.empty else ''
        color_line=(base/'config/kdeglobals').read_text().split('[Colors:Window]',1)[1].split('BackgroundNormal=',1)[1].splitlines()[0]
        channels=color_line.split(',');env['WEATHER_ALPHA']=channels[3] if len(channels)==4 else '255'
        with (output/'verification.log').open('w') as log_file:
            result=subprocess.run(['dbus-run-session','--','env','LD_PRELOAD='+str(helper),str(args.binary.resolve())],env=env,stdout=log_file,stderr=log_file,timeout=35)
        log=(output/'verification.log').read_text()
        print('Exit:',result.returncode);print(log[-7000:])
        assert result.returncode==0 and 'PASS:' in log and 'FAIL:' not in log
        assert not any(s in log for s in ('ReferenceError:', 'TypeError:', 'Binding loop detected', 'Cannot assign', 'Unable to assign', 'is not a type'))

if __name__=='__main__':main()
