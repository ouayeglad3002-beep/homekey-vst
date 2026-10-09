"""Genere le pack 'HomeKey Preset' (format HomeKeys .hkpreset) + le zip integre au plugin.
Usage : python tools/make_presets.py   (depuis la racine du depot)"""
import os, shutil, zipfile
from xml.sax.saxutils import quoteattr

# type : 0 Doux, 1 Grave, 2 Orchestre, 3 Cinematique, 4 Dreaming
D = dict(type=0, volume=0.0, tone=0.0, velocity=0.75, release=0.5, layer=0.0, chorus=0.1, reverb=0.3, size=0.6, width=0.7,
         octave=0, drive=0.0, wow=0.0, crush=0.0, vinyl=0.0,
         fcut=20000, fres=0.1, ftype=0, attack=0.002, decay=1.0, hammer=1.0, sub=0.0, unison=1.0, fine=0.0,
         dtime=1, dfb=0.35, dmix=0.0, cdepth=0.0, crate=0.25,
         chop=0, choptype=0, choprate=2, chopmix=1.0)
STARS_X = [0.50, 0.78, 0.86, 0.66, 0.38, 0.16, 0.24, 0.55]
STARS_Y = [0.18, 0.28, 0.58, 0.82, 0.78, 0.55, 0.30, 0.50]

def P(name, **kw): return (name, kw)

PACK = {
 "01 - Keys": [
  P("Coton Felt [Douceur]", type=0, tone=-.35, velocity=.6, release=.6, chorus=.15, reverb=.3, size=.5, width=.65, drive=.1, wow=.05, vinyl=.05),
  P("Matin Calme [Douceur]", type=0, tone=-.1, velocity=.7, chorus=.05, reverb=.22, size=.35, width=.6, drive=.05),
  P("Velours Intime [Douceur]", type=0, volume=1, tone=-.55, velocity=.5, release=.8, chorus=0, reverb=.18, size=.25, width=.5, drive=.18, vinyl=.08),
  P("Rose Velours [Love]", type=0, tone=-.15, velocity=.7, release=.9, chorus=.35, reverb=.35, width=.8, drive=.12, wow=.1, vinyl=.04),
  P("Slow Jam 3AM [RnB]", type=4, volume=-.5, tone=-.05, release=1.2, layer=.25, chorus=.5, reverb=.45, size=.7, width=.9, drive=.15, wow=.12),
  P("Neo Soul Warm [Soul]", type=0, tone=.05, velocity=.8, release=.6, chorus=.3, reverb=.28, size=.5, drive=.3, wow=.1, vinyl=.1),
  P("Sunday Church [Gospel]", type=2, volume=-1, tone=.25, velocity=.9, release=.7, layer=.25, reverb=.42, size=.85, width=.8, drive=.12),
  P("Radio Bright [Pop]", type=2, volume=-1, tone=.55, velocity=.85, release=.4, layer=.15, chorus=.15, reverb=.28, size=.55, width=.85, drive=.15),
  P("Amapiano Log [Afro]", type=2, volume=-1, tone=.2, velocity=.8, release=.45, layer=.1, chorus=.25, reverb=.32, width=.8, drive=.2, decay=.6),
  P("Wedding Lights [Love]", type=2, volume=-1, tone=.2, velocity=.8, release=1.0, layer=.5, reverb=.4, size=.8, width=.85),
 ],
 "02 - Bass": [
  P("Sub Felt Bass [Trap]", type=1, octave=-1, tone=-.6, velocity=.85, release=.35, layer=0, chorus=0, reverb=.08, size=.3, width=.15, sub=.8, drive=.3, fcut=900, fres=.2, hammer=.6),
  P("808 Piano Bass [Trap]", type=1, octave=-2, volume=-1, tone=-.4, velocity=.9, release=.5, chorus=0, reverb=.05, width=.1, sub=1.0, drive=.55, fcut=1200, decay=1.6),
  P("Cold Stairwell [Drill]", type=1, octave=-1, tone=-.55, velocity=.85, release=.7, layer=.6, chorus=0, reverb=.55, size=.9, width=.65, drive=.25, crush=.15),
  P("Dark Bounce [Trap]", type=1, octave=-1, tone=-.45, velocity=.9, release=.35, layer=.5, chorus=.05, reverb=.25, size=.55, width=.6, drive=.35, crush=.05),
  P("Abysse Bass [Dark]", type=1, octave=-2, volume=-2.5, tone=-.8, velocity=.85, release=2.2, layer=.9, chorus=.2, reverb=.6, size=.95, width=.9, drive=.3, wow=.2, crush=.2, vinyl=.1),
  P("Tape Wobble Bass [Lo-Fi]", type=0, octave=-1, tone=-.3, velocity=.8, release=.4, chorus=0, reverb=.1, width=.2, sub=.6, drive=.45, wow=.7, vinyl=.2, fcut=1500),
 ],
 "03 - Leads": [
  P("Rage Lead [Rage]", type=2, volume=-1.5, tone=.6, velocity=.95, release=.3, layer=.2, chorus=.3, reverb=.3, width=1, drive=.6, crush=.25, unison=4, dmix=.25, dtime=2),
  P("Pluggnb Glow [Pluggnb]", type=4, volume=-1, tone=.15, velocity=.8, release=.6, layer=.3, chorus=.55, reverb=.4, size=.7, width=.95, drive=.18, wow=.12, crush=.12, dmix=.2),
  P("Unison Keys Lead [Pop]", type=2, volume=-2, tone=.4, velocity=.85, release=.4, chorus=.2, reverb=.25, width=1, unison=5.5, drive=.25, octave=1, dmix=.18, dtime=1),
  P("Filter Scream [Alien]", type=2, volume=-3, tone=.5, velocity=.9, release=.35, width=.9, drive=.7, fcut=1800, fres=.85, unison=3, cdepth=.6, crate=1.5),
  P("Love Lead [Love Trap]", type=1, tone=.05, velocity=.8, release=.7, layer=.2, chorus=.3, reverb=.4, size=.7, width=.85, drive=.2, wow=.08, octave=1, dmix=.3, dtime=2, dfb=.45),
  P("Delay Dot Lead [Trap]", type=0, volume=1, tone=.2, velocity=.85, release=.4, chorus=.1, reverb=.25, width=.8, drive=.25, octave=1, dmix=.45, dtime=2, dfb=.55),
 ],
 "04 - Bells": [
  P("Bell Tower [Trap]", type=2, tone=.45, velocity=.85, release=.8, chorus=.25, reverb=.45, size=.8, width=.9, octave=1, drive=.15, crush=.1),
  P("Boite a Musique [Douceur]", type=2, volume=1, tone=.5, velocity=.75, release=1.2, chorus=.2, reverb=.5, size=.75, width=.9, octave=2, wow=.05, vinyl=.05),
  P("Cristal Hiver [Dream]", type=4, volume=-1.5, tone=.45, velocity=.7, release=2.0, layer=.4, chorus=.5, reverb=.6, size=.9, width=1, octave=1),
  P("Astral Glass [Dream]", type=4, volume=-1, tone=.25, velocity=.7, release=2.5, layer=.55, chorus=.65, reverb=.75, size=.98, width=1, octave=1, wow=.05),
  P("Trap Bells HP [Trap]", type=2, volume=1, tone=.6, velocity=.9, release=.6, reverb=.35, width=.9, octave=1, ftype=2, fcut=600, fres=.3, hammer=1.8, dmix=.25, dtime=1),
  P("Ice Chimes [Cinema]", type=4, volume=-1, tone=.7, velocity=.75, release=3.0, layer=.3, chorus=.4, reverb=.7, size=.97, width=1, octave=2, decay=.6, dmix=.3, dtime=4, dfb=.5),
 ],
 "05 - Pads": [
  P("Interstellaire [Cinema]", type=3, volume=-1, tone=-.15, release=2.6, layer=.65, chorus=.35, reverb=.72, size=.97, width=1),
  P("Nuage Rose [Dream]", type=4, tone=-.25, velocity=.6, release=2.0, layer=.7, chorus=.6, reverb=.65, size=.95, width=1, drive=.05, wow=.15, vinyl=.05),
  P("Sleep Paralysis [Dark]", type=4, volume=-.5, tone=-.55, velocity=.7, release=3.2, layer=.8, chorus=.75, reverb=.8, size=.99, width=1, octave=-1, drive=.1, wow=.3, crush=.05, vinyl=.1),
  P("Berceuse Lunaire [Douceur]", type=4, volume=-1, tone=-.4, velocity=.55, release=2.2, layer=.35, chorus=.4, reverb=.55, size=.85, width=.9, wow=.08, vinyl=.03),
  P("Slow Strings [Orchestre]", type=2, volume=-1, tone=-.1, velocity=.7, release=2.0, layer=1.0, chorus=.15, reverb=.55, size=.9, width=.95, attack=.6, hammer=.3),
  P("Orchestral Swell [Cinema]", type=3, volume=-1.5, tone=0, velocity=.8, release=2.5, layer=.9, chorus=.2, reverb=.65, size=.95, width=1, attack=1.2, hammer=.2),
  P("Ouverture Epique [Cinema]", type=3, volume=-1.5, tone=.1, velocity=.85, release=1.6, layer=.8, chorus=.15, reverb=.6, size=.92, width=.95, drive=.05),
 ],
 "06 - Plucks": [
  P("Short Pluck [Pop]", type=2, volume=1, tone=.3, velocity=.85, release=.15, reverb=.25, width=.8, decay=.2, hammer=1.5, dmix=.2, dtime=1),
  P("Drill Pluck [Drill]", type=1, volume=1, tone=-.1, velocity=.9, release=.15, reverb=.35, size=.8, width=.7, decay=.25, drive=.35, crush=.1, fcut=3500, fres=.4),
  P("Afro Pluck [Afro]", type=2, volume=1, tone=.35, velocity=.85, release=.2, chorus=.3, reverb=.3, width=.85, decay=.3, hammer=1.4, dmix=.22, dtime=0, dfb=.3),
  P("Lofi Pluck [Lo-Fi]", type=0, volume=1.5, tone=-.2, velocity=.8, release=.2, reverb=.3, width=.6, decay=.3, drive=.35, wow=.35, crush=.3, vinyl=.35),
  P("Glass Pluck [Dream]", type=4, volume=-.5, tone=.5, velocity=.8, release=.3, chorus=.4, reverb=.6, size=.9, width=1, decay=.25, octave=1, dmix=.35, dtime=2, dfb=.5),
 ],
 "07 - Lo-Fi": [
  P("Cassette Chill [Chill]", type=0, tone=-.35, velocity=.65, release=.6, chorus=.2, reverb=.3, size=.5, width=.6, drive=.35, wow=.45, crush=.25, vinyl=.45),
  P("Pluie sur Vinyle [Chill]", type=0, tone=-.5, velocity=.6, release=.9, chorus=.1, reverb=.35, width=.7, drive=.25, wow=.3, crush=.15, vinyl=.75),
  P("Study Room 92 [Boom Bap]", type=2, volume=-1, tone=-.25, velocity=.7, release=.5, chorus=.15, reverb=.25, size=.4, width=.55, drive=.45, wow=.35, crush=.45, vinyl=.4),
  P("VHS Memories [Chill]", type=4, volume=-1, tone=-.3, velocity=.65, release=1.4, layer=.3, chorus=.45, reverb=.45, size=.75, width=.85, drive=.3, wow=.75, crush=.35, vinyl=.35),
  P("Jazz Club Smoke [Jazz]", type=2, volume=-1, tone=-.1, velocity=.85, release=.5, chorus=.05, reverb=.3, size=.5, width=.6, drive=.25, wow=.08, crush=.05, vinyl=.25),
 ],
 "08 - Cinematic": [
  P("Coeur Brise [Love]", type=3, volume=-1, tone=-.25, velocity=.7, release=1.6, layer=.4, chorus=.2, reverb=.5, size=.85, width=.85, drive=.08, wow=.06, vinyl=.06),
  P("Derniere Scene [Film]", type=1, tone=-.2, velocity=.75, release=1.8, layer=.55, chorus=.1, reverb=.55, size=.9, width=.85, drive=.05),
  P("Thriller Pulse [Film]", type=1, tone=-.5, velocity=.9, release=.5, layer=.7, chorus=.05, reverb=.4, size=.8, width=.75, octave=-1, drive=.2, dmix=.3, dtime=1, dfb=.5),
  P("NY Slide [Drill]", type=3, volume=-1, tone=-.15, velocity=.9, release=.5, layer=.45, chorus=.1, reverb=.45, size=.8, width=.8, drive=.35, wow=.06, crush=.05),
  P("UK Grey Keys [Drill]", type=1, tone=-.3, velocity=.85, release=.4, layer=.35, chorus=.05, reverb=.38, size=.75, width=.7, drive=.3, wow=.04, crush=.1, vinyl=.02),
 ],
 "09 - Chops": [
  P("Stutter Love [Love]", type=0, tone=-.1, release=.8, chorus=.3, reverb=.35, width=.8, drive=.15, wow=.1, chop=1, choptype=2, choprate=2),
  P("Reverse Dream [Dream]", type=4, volume=-1, release=1.5, layer=.4, chorus=.5, reverb=.55, size=.9, width=1, chop=1, choptype=3, choprate=1),
  P("Tape Stop Trap [Trap]", type=1, tone=-.3, velocity=.9, release=.6, layer=.3, reverb=.3, width=.7, drive=.3, chop=1, choptype=4, choprate=2),
  P("Glitch Android [Alien]", type=2, volume=-1.5, tone=.3, velocity=.9, release=.6, layer=.2, chorus=.5, reverb=.35, width=.9, drive=.55, wow=.4, crush=.6, vinyl=.15, chop=1, choptype=7, choprate=2),
  P("Gate Trance Keys [EDM]", type=2, volume=-1, tone=.3, velocity=.85, release=1.2, layer=.5, chorus=.3, reverb=.45, size=.85, width=1, chop=1, choptype=1, choprate=2),
  P("Half Speed Lofi [Lo-Fi]", type=0, tone=-.3, velocity=.7, release=.8, chorus=.2, reverb=.3, width=.6, drive=.3, wow=.3, crush=.2, vinyl=.4, chop=1, choptype=5, choprate=2, chopmix=.8),
  P("Octave Chop [Pluggnb]", type=4, volume=-1, tone=.1, release=.7, layer=.3, chorus=.5, reverb=.4, width=.95, drive=.15, chop=1, choptype=6, choprate=2, chopmix=.7),
 ],
 "10 - Constellation": [
  P("Xeno Signal [Alien]", type=4, volume=-1, release=1.8, layer=.6, chorus=.8, reverb=.6, size=.9, width=1, drive=.4, wow=.6, crush=.55, cdepth=.7, crate=.6),
  P("Living Nebula [Ambient]", type=4, volume=-1, tone=-.1, velocity=.65, release=2.5, layer=.7, chorus=.5, reverb=.7, size=.97, width=1, cdepth=.8, crate=.15),
  P("Breathing Felt [Douceur]", type=0, tone=-.3, velocity=.6, release=.8, chorus=.2, reverb=.35, width=.7, drive=.15, wow=.1, vinyl=.1, cdepth=.45, crate=.12),
  P("Alien Drift [Dark]", type=1, tone=-.4, velocity=.85, release=1.4, layer=.6, chorus=.3, reverb=.55, size=.9, width=.9, drive=.3, cdepth=.75, crate=.35),
  P("Cosmic Strings [Cinema]", type=3, volume=-1.5, release=2.2, layer=.85, chorus=.3, reverb=.65, size=.95, width=1, attack=.4, cdepth=.6, crate=.25),
  P("Broken Android [Alien]", type=2, volume=-1.5, tone=.3, velocity=.9, release=.6, layer=.2, chorus=.5, reverb=.35, size=.7, width=.9, drive=.55, wow=.4, crush=.85, vinyl=.15, cdepth=.5, crate=1.2),
 ],
}

def fmt(v):
    v = float(v)
    return str(int(v)) if v.is_integer() else repr(round(v, 4))

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
pack = os.path.join(root_dir, "HomeKey Preset")
shutil.rmtree(pack, ignore_errors=True)
count = 0
for folder, presets in PACK.items():
    os.makedirs(os.path.join(pack, folder), exist_ok=True)
    for name, kw in presets:
        vals = dict(D); vals.update(kw)
        assert set(kw) <= set(D), (name, set(kw) - set(D))
        for i in range(8):
            vals[f"s{i}x"] = STARS_X[i]; vals[f"s{i}y"] = STARS_Y[i]
        params = "\n".join(f'  <PARAM id="{k}" value="{fmt(v)}"/>' for k, v in sorted(vals.items()))
        xml = f'<?xml version="1.0" encoding="UTF-8"?>\n\n<HomeKeysState presetName={quoteattr(name)}>\n{params}\n</HomeKeysState>\n'
        with open(os.path.join(pack, folder, name + ".hkpreset"), "w", encoding="utf-8") as f:
            f.write(xml)
        count += 1

with open(os.path.join(pack, "LISEZ-MOI.txt"), "w", encoding="utf-8") as f:
    f.write("HOMEKEY PRESET - pack officiel HomeKeys v2\n"
            "Ce pack s'installe TOUT SEUL au premier lancement de HomeKeys v2.\n"
            "Pour ajouter d'autres presets : bouton IMPORT dans HomeKeys.\n"
            "Le style est indique entre crochets : [Trap], [Love], [Lo-Fi]...\n")

os.makedirs(os.path.join(root_dir, "Resources"), exist_ok=True)
zpath = os.path.join(root_dir, "Resources", "HomeKeyPresets.zip")
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    for dp, _, files in os.walk(pack):
        for fn in sorted(files):
            full = os.path.join(dp, fn)
            z.write(full, os.path.relpath(full, root_dir))
print(count, "presets ->", pack, "+", zpath)
