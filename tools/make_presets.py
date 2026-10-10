"""Genere le pack 'HomeKey Preset' de HomeKey I (format .hkpreset) + le zip integre au plugin.
Usage : python tools/make_presets.py   (depuis la racine du depot)"""
import json, math, os, random, shutil, zipfile
from xml.sax.saxutils import quoteattr

# mondes : 0 DREAMING, 1 NEBULA, 2 CINEMA, 3 ABYSS, 4 BACKROOMS
D = dict(type=0, volume=0.0, tone=0.0, velocity=0.75, release=0.6, layer=0.3, chorus=0.15, reverb=0.35, size=0.7, width=0.75,
         octave=0, drive=0.0, wow=0.0, crush=0.0, vinyl=0.0,
         piano=1.0, harm=0.25, metal=0.0, ring=0.0, tex=0, texlvl=0.5,
         fcut=20000, fres=0.1, ftype=0, attack=0.002, decay=1.0, hammer=1.0, sub=0.0, unison=1.0, fine=0.0,
         dtime=1, dfb=0.35, dmix=0.0, shimmer=0.0, air=0.0, hum=0.0,
         cdepth=0.0, crate=0.25, cchaos=0.3, corbit=0.0, cvar=0.0, cmut=0.7,
         chop=0, choptype=0, choprate=2, chopmix=1.0)

# cibles : 0 TONE 1 FILTRE 2 RESO 3 DRIVE 4 WOW 5 CRUSH 6 CHORUS 7 REVERB 8 NAPPE 9 ESPACE
#          10 METAL 11 ANNEAU 12 SHIMMER 13 PITCH 14 VOLUME 15 TEXTURE 16 AIR 17 DELAY
FAV = {0: [12, 7, 8, 0, 6, 9, 13, 16], 1: [11, 10, 1, 13, 12, 16, 17, 15], 2: [8, 7, 1, 14, 0, 15, 9, 12],
       3: [3, 1, 11, 10, 2, 5, 14, 15], 4: [4, 5, 1, 14, 13, 0, 7, 16]}
SHAPES = {0: [0, 0, 4, 1], 1: [0, 1, 2, 3, 4], 2: [0, 1, 4], 3: [1, 2, 3, 4], 4: [1, 2, 2, 3]}
# tex : 0 aucune 1 gong 2 cymbale archet 3 vibes archet 4 cloche tube 5 enclume 6 frein archet 7 cloche nepal

def stars(world, seed):
    r = random.Random(seed)
    start = r.random() * math.tau
    fav = FAV[world][:]
    r.shuffle(fav)
    out = {}
    for i in range(8):
        a = start + math.tau * (i + 0.6 * r.random()) / 8
        rad = 0.10 + 0.35 * r.random()
        out[f"s{i}x"] = round(min(0.97, max(0.03, 0.5 + rad * math.cos(a))), 3)
        out[f"s{i}y"] = round(min(0.97, max(0.03, 0.5 + rad * math.sin(a))), 3)
        out[f"s{i}t"] = fav[i % len(fav)]
        out[f"s{i}w"] = r.choice(SHAPES[world])
    return out

def P(name, **kw): return (name, kw)

PACK = {
 "01 - Pianos de Reve": [
  P("Lucid Felt", type=0, tone=-.35, velocity=.65, release=2.0, layer=.45, chorus=.4, reverb=.6, size=.9, width=.95, shimmer=.35, harm=.15, wow=.08, cdepth=.2, crate=.12),
  P("Somnia Grand", type=0, tone=-.1, release=2.5, layer=.5, chorus=.35, reverb=.65, size=.95, width=1, shimmer=.55, cdepth=.15, crate=.08),
  P("Halo Lullaby", type=0, volume=1, tone=-.5, velocity=.55, release=2.8, layer=.6, chorus=.5, reverb=.7, size=.97, width=1, shimmer=.45, octave=1, harm=.1, cdepth=.25, crate=.1),
  P("Opaline Rain", type=0, tone=0, release=1.6, layer=.35, chorus=.45, reverb=.55, width=.95, shimmer=.3, dmix=.3, dtime=4, dfb=.5, cdepth=.2, crate=.2),
  P("Velvet Dream", type=0, tone=-.4, velocity=.6, release=1.8, layer=.4, chorus=.3, reverb=.5, size=.85, width=.9, drive=.15, wow=.15, vinyl=.08, shimmer=.25, cdepth=.15, crate=.1),
  P("Nuage Infini", type=0, tone=-.2, release=3.5, layer=.8, chorus=.6, reverb=.75, size=.99, width=1, attack=.25, shimmer=.7, air=.2, cdepth=.3, crate=.06),
  P("Aurora Keys", type=0, tone=.15, release=1.5, layer=.3, chorus=.4, reverb=.55, size=.9, width=1, shimmer=.4, tex=3, texlvl=.25, cdepth=.2, crate=.15),
  P("Sleepwalker", type=0, volume=1, tone=-.6, velocity=.5, release=2.2, layer=.5, chorus=.5, reverb=.6, size=.9, width=.9, wow=.3, shimmer=.35, cvar=.3, cdepth=.3, crate=.1),
 ],
 "02 - Espace Alien": [
  P("Xeno Signal", type=1, tone=.1, release=1.8, layer=.4, chorus=.5, reverb=.6, size=.9, width=1, harm=.6, metal=.55, ring=.35, tex=3, texlvl=.35, air=.35, shimmer=.25, dmix=.25, dtime=2, cdepth=.5, crate=.6, cchaos=.6),
  P("Nebula Choir", type=1, tone=-.1, release=2.6, layer=.7, chorus=.6, reverb=.7, size=.97, width=1, attack=.4, harm=.4, metal=.3, tex=2, texlvl=.3, shimmer=.45, air=.4, cdepth=.4, crate=.25),
  P("Quasar Bells", type=1, volume=-1, tone=.4, release=2.2, layer=.2, chorus=.4, reverb=.6, size=.9, width=1, octave=1, harm=.7, metal=.85, ring=.2, dmix=.35, dtime=2, dfb=.55, cdepth=.35, crate=.5),
  P("Zeta Transmission", type=1, volume=-1, tone=.2, release=1.0, layer=.2, chorus=.3, reverb=.45, width=1, piano=.6, harm=.6, ring=.6, fcut=5000, fres=.45, crush=.25, air=.3, dmix=.3, dtime=0, cdepth=.6, crate=1.2, cchaos=.7),
  P("Void Drifter", type=1, tone=-.3, release=3.0, layer=.6, chorus=.5, reverb=.75, size=.99, width=1, harm=.5, metal=.6, tex=6, texlvl=.4, air=.6, shimmer=.3, octave=-1, cdepth=.5, crate=.15, corbit=.4),
  P("Pulsar Grid", type=1, volume=-1, tone=.3, release=.6, layer=.1, chorus=.2, reverb=.4, width=.9, harm=.6, metal=.5, ring=.3, dmix=.4, dtime=0, dfb=.6, cdepth=.6, crate=2.0, cchaos=.5),
  P("Orbit 7", type=1, tone=0, release=1.6, layer=.4, chorus=.45, reverb=.55, size=.85, width=1, harm=.5, metal=.4, ring=.15, shimmer=.3, air=.25, cdepth=.55, crate=.4, corbit=.8, cvar=.3),
  P("Alien Lullaby", type=1, volume=1, tone=-.2, velocity=.6, release=2.0, layer=.4, chorus=.55, reverb=.6, width=1, octave=1, harm=.5, metal=.7, tex=7, texlvl=.3, shimmer=.4, cdepth=.35, crate=.2),
 ],
 "03 - Cinematique Lourd": [
  P("Titan Grand", type=2, tone=0, velocity=.85, release=2.5, layer=.7, chorus=.1, reverb=.6, size=.95, width=.95, sub=.5, harm=.25, tex=1, texlvl=.3, cdepth=.12, crate=.08),
  P("Monolith", type=2, tone=-.3, velocity=.9, release=3.0, layer=.85, chorus=.1, reverb=.7, size=.99, width=1, octave=-1, sub=.7, drive=.15, tex=1, texlvl=.5, cdepth=.15, crate=.06),
  P("Requiem Strings", type=2, tone=-.1, release=3.0, layer=1.0, chorus=.15, reverb=.65, size=.97, width=1, attack=.7, hammer=.3, piano=.7, sub=.3, cdepth=.2, crate=.07),
  P("Eclipse Hit", type=2, volume=-1.5, tone=.1, velocity=.95, release=2.0, layer=.6, reverb=.6, size=.95, width=1, sub=.8, drive=.25, tex=4, texlvl=.45, hammer=1.6, shimmer=.15),
  P("Exodus", type=2, tone=-.2, release=2.8, layer=.75, chorus=.2, reverb=.7, size=.98, width=1, sub=.4, shimmer=.35, air=.2, tex=3, texlvl=.2, cdepth=.25, crate=.1),
  P("Odyssey Low", type=2, tone=-.4, velocity=.85, release=2.2, layer=.8, chorus=.1, reverb=.6, size=.95, width=.9, octave=-1, sub=.6, harm=.35, metal=.2, dmix=.2, dtime=4, cdepth=.15, crate=.1),
  P("Empire Theme", type=2, tone=.2, velocity=.9, release=1.8, layer=.65, chorus=.15, reverb=.55, size=.9, width=.95, sub=.35, tex=4, texlvl=.25, dmix=.15, dtime=3),
  P("Last Light", type=2, tone=-.25, velocity=.7, release=3.2, layer=.6, chorus=.25, reverb=.75, size=.99, width=1, sub=.2, shimmer=.45, cdepth=.2, crate=.08),
 ],
 "04 - Basses Lourdes & Metal": [
  P("Iron Abyss Bass", type=3, octave=-1, volume=-3, tone=-.5, velocity=.9, release=.8, layer=.3, chorus=0, reverb=.2, size=.6, width=.4, sub=1.0, drive=.55, metal=.7, harm=.6, ring=.25, fcut=2500, fres=.35),
  P("Anvil Sub", type=3, octave=-1, volume=-3, tone=-.4, velocity=.95, release=.6, layer=.1, reverb=.15, width=.3, sub=.9, drive=.5, tex=5, texlvl=.6, metal=.5, harm=.5, fcut=3500),
  P("Obsidian 808", type=3, octave=-2, volume=-3, tone=-.6, velocity=.9, release=1.2, layer=0, chorus=0, reverb=.1, width=.2, sub=1.0, drive=.7, decay=1.8, piano=.6, harm=.4, metal=.3, fcut=1500, fres=.2),
  P("Steel Growl", type=3, octave=-1, volume=-3.5, tone=-.2, velocity=.9, release=.7, layer=.2, reverb=.2, width=.6, sub=.7, drive=.65, ring=.55, metal=.8, harm=.7, fcut=1800, fres=.6, cdepth=.5, crate=.8),
  P("Dragon Iron", type=3, octave=-1, volume=-3, tone=-.3, velocity=.95, release=1.4, layer=.5, chorus=.1, reverb=.35, size=.85, width=.8, sub=.8, drive=.5, metal=.9, harm=.7, tex=1, texlvl=.45, fcut=4000),
  P("Rust Engine", type=3, octave=-1, volume=-3, tone=-.4, velocity=.9, release=.5, layer=.1, reverb=.15, width=.5, sub=.7, drive=.6, crush=.35, ring=.4, metal=.6, harm=.6, fcut=2200, fres=.4, cdepth=.45, crate=1.5, cchaos=.6),
  P("Necro Pulse", type=3, octave=-1, volume=-3, tone=-.5, velocity=.9, release=.6, layer=.2, reverb=.25, width=.6, sub=.8, drive=.5, metal=.6, harm=.5, chop=1, choptype=1, choprate=2, chopmix=.9),
  P("Throne Bass", type=3, octave=-2, volume=-2.5, tone=-.3, velocity=.85, release=1.5, layer=.4, reverb=.3, size=.8, width=.6, sub=.9, drive=.4, metal=.5, harm=.4, tex=4, texlvl=.35, fcut=3000),
  P("Tungsten Drop", type=3, octave=-1, volume=-3, tone=-.3, velocity=.95, release=1.0, layer=.2, reverb=.2, width=.5, sub=.8, drive=.55, metal=1.0, harm=.8, ring=.3, decay=.6, fcut=5000, fres=.3),
 ],
 "05 - Dark Fantasy": [
  P("Wraith Cathedral", type=3, tone=-.4, release=2.6, layer=.8, chorus=.2, reverb=.7, size=.98, width=1, sub=.5, drive=.2, metal=.5, harm=.5, tex=4, texlvl=.4, shimmer=.2, cdepth=.25, crate=.1),
  P("Rune Keeper", type=3, tone=-.3, velocity=.8, release=2.0, layer=.6, chorus=.15, reverb=.6, size=.95, width=.95, sub=.4, metal=.6, harm=.5, tex=7, texlvl=.4, dmix=.25, dtime=4, cdepth=.2, crate=.15),
  P("Blood Moon", type=3, tone=-.6, release=2.4, layer=.7, chorus=.3, reverb=.65, size=.95, width=1, sub=.6, drive=.3, ring=.2, metal=.4, harm=.4, wow=.15, cdepth=.35, crate=.12),
  P("Dragon Lair", type=3, octave=-1, volume=-2, tone=-.5, velocity=.9, release=2.8, layer=.9, chorus=.1, reverb=.7, size=.99, width=1, sub=.9, drive=.35, metal=.5, harm=.5, tex=1, texlvl=.6, air=.2),
  P("Cursed Music Box", type=3, volume=1, tone=.1, velocity=.75, release=1.8, layer=.2, chorus=.4, reverb=.6, size=.9, width=1, octave=2, metal=.5, harm=.5, wow=.35, vinyl=.25, crush=.1, cvar=.35, cdepth=.3, crate=.3),
  P("Black Citadel", type=3, tone=-.5, velocity=.9, release=2.2, layer=.75, reverb=.6, size=.95, width=.9, octave=-1, sub=.6, drive=.35, metal=.7, harm=.6, ring=.3, tex=6, texlvl=.4, fcut=6000, cdepth=.3, crate=.2),
  P("Elven Ruins", type=0, tone=-.1, release=2.6, layer=.6, chorus=.45, reverb=.7, size=.97, width=1, metal=.35, harm=.35, tex=3, texlvl=.35, shimmer=.45, air=.25, cdepth=.25, crate=.1),
 ],
 "06 - Backrooms": [
  P("Level 0 Hum", type=4, tone=-.4, velocity=.7, release=1.0, layer=.2, chorus=.3, reverb=.4, size=.35, width=.6, hum=.6, wow=.45, crush=.2, vinyl=.3, fcut=3200, unison=2.5, cdepth=.35, crate=.8, cchaos=.8),
  P("Fluorescent Hallway", type=4, tone=-.3, release=.9, layer=.25, chorus=.35, reverb=.45, size=.45, width=.7, hum=.75, wow=.5, crush=.3, vinyl=.25, fcut=2600, fres=.25, unison=3, fine=-18, cdepth=.4, crate=1.2, cchaos=.9),
  P("Wet Carpet", type=4, volume=1, tone=-.65, velocity=.65, release=1.2, layer=.3, chorus=.25, reverb=.35, size=.3, width=.5, hum=.4, wow=.6, crush=.15, vinyl=.5, fcut=1800, drive=.2),
  P("Liminal Lobby", type=4, tone=-.2, release=1.6, layer=.3, chorus=.4, reverb=.5, size=.55, width=.75, hum=.35, wow=.35, vinyl=.35, crush=.1, shimmer=.1, air=.15, fcut=4200, cvar=.3, cdepth=.3, crate=.5),
  P("Exit 404", type=4, tone=-.5, velocity=.75, release=.8, layer=.2, reverb=.35, size=.35, width=.6, hum=.5, wow=.7, crush=.45, vinyl=.4, drive=.3, fcut=2400, fres=.35, chop=1, choptype=4, choprate=2, chopmix=.7),
  P("Mall at 3AM", type=4, tone=-.15, release=1.4, layer=.35, chorus=.5, reverb=.55, size=.6, width=.8, hum=.3, wow=.4, vinyl=.3, dmix=.25, dtime=3, fcut=3800, unison=2, fine=12),
  P("Noclip", type=4, volume=-1, tone=-.3, release=1.0, layer=.2, chorus=.3, reverb=.4, size=.4, width=.7, hum=.55, wow=.55, crush=.55, vinyl=.3, ring=.15, fcut=3000, fres=.3, cdepth=.6, crate=1.8, cchaos=1.0),
 ],
 "07 - Cristal & Cloches": [
  P("Crystal Cavern", type=0, volume=-1, tone=.4, release=2.5, layer=.3, chorus=.5, reverb=.7, size=.97, width=1, octave=1, metal=.6, harm=.6, tex=3, texlvl=.35, shimmer=.5, cdepth=.2, crate=.2),
  P("Glass Cathedral", type=2, volume=-1, tone=.3, release=2.5, layer=.4, reverb=.7, size=.98, width=1, octave=1, metal=.7, harm=.6, tex=4, texlvl=.5, shimmer=.3),
  P("Ice Prism", type=1, volume=-1, tone=.5, release=2.0, layer=.2, chorus=.45, reverb=.6, size=.9, width=1, octave=2, metal=.8, harm=.7, dmix=.35, dtime=2, dfb=.5, shimmer=.35, cdepth=.25, crate=.4),
  P("Diamond Dust", type=0, tone=.6, velocity=.7, release=1.6, layer=.1, chorus=.4, reverb=.55, width=1, octave=2, metal=.55, harm=.5, decay=.5, dmix=.4, dtime=0, dfb=.55, shimmer=.4),
  P("Nepal Bowl", type=1, tone=0, release=2.8, layer=.3, chorus=.3, reverb=.65, size=.95, width=1, tex=7, texlvl=.7, metal=.5, harm=.4, piano=.6, shimmer=.25),
  P("Gong Ritual", type=3, tone=-.2, release=3.0, layer=.4, reverb=.7, size=.98, width=1, tex=1, texlvl=.75, metal=.6, harm=.4, piano=.6, sub=.4),
 ],
 "08 - Leads & Plucks": [
  P("Astral Lead", type=1, volume=-1.5, tone=.4, release=.6, layer=.1, chorus=.4, reverb=.45, width=1, octave=1, harm=.6, metal=.3, unison=4, drive=.25, dmix=.35, dtime=2, dfb=.5, cdepth=.3, crate=.6),
  P("Dream Pluck", type=0, volume=1, tone=.2, release=.3, chorus=.4, reverb=.55, size=.9, width=1, decay=.3, shimmer=.35, dmix=.3, dtime=2, dfb=.5),
  P("Metal Pluck", type=3, tone=0, velocity=.9, release=.3, reverb=.4, size=.8, width=.8, decay=.3, metal=.7, harm=.6, ring=.2, drive=.3, dmix=.25, dtime=1),
  P("Backroom Pluck", type=4, volume=1, tone=-.3, release=.25, reverb=.3, size=.3, width=.6, decay=.3, wow=.4, crush=.3, vinyl=.3, hum=.2),
  P("Neon Lead", type=1, volume=-2, tone=.5, velocity=.9, release=.4, chorus=.3, reverb=.35, width=1, octave=1, harm=.7, ring=.35, fcut=6000, fres=.5, unison=3.5, drive=.35, cdepth=.4, crate=1.0),
  P("Ghost Pluck", type=0, tone=-.2, release=.4, chorus=.5, reverb=.65, size=.95, width=1, decay=.35, wow=.2, shimmer=.5, air=.2, cvar=.4),
 ],
 "09 - Chops": [
  P("Stutter Nebula", type=1, release=1.2, layer=.3, chorus=.4, reverb=.5, width=1, harm=.5, metal=.4, chop=1, choptype=2, choprate=2),
  P("Reverse Dream", type=0, release=1.8, layer=.4, chorus=.5, reverb=.6, size=.9, width=1, shimmer=.4, chop=1, choptype=3, choprate=1),
  P("Tape Stop Abyss", type=3, octave=-1, volume=-2, tone=-.4, release=1.0, layer=.4, reverb=.3, width=.7, sub=.7, drive=.4, metal=.5, chop=1, choptype=4, choprate=2),
  P("Glitch Android", type=1, volume=-1.5, tone=.3, release=.6, layer=.2, chorus=.5, reverb=.35, width=.9, drive=.45, wow=.3, crush=.5, ring=.3, chop=1, choptype=7, choprate=2),
  P("Half Speed Room", type=4, tone=-.3, release=.8, chorus=.2, reverb=.35, size=.4, width=.6, hum=.4, wow=.4, vinyl=.4, crush=.2, chop=1, choptype=5, choprate=2, chopmix=.8),
 ],
 "10 - Constellation Vivante": [
  P("Living Nebula", type=1, tone=-.1, velocity=.65, release=2.5, layer=.6, chorus=.5, reverb=.7, size=.97, width=1, harm=.5, metal=.4, shimmer=.35, air=.3, cdepth=.85, crate=.2, cchaos=.5, corbit=.3),
  P("Breathing Felt", type=0, tone=-.3, velocity=.6, release=1.2, layer=.3, chorus=.3, reverb=.45, width=.8, drive=.15, wow=.1, vinyl=.1, cdepth=.6, crate=.12, cchaos=.3),
  P("Storm Engine", type=3, volume=-2, tone=-.3, release=1.4, layer=.5, reverb=.5, size=.9, width=.9, sub=.6, drive=.45, metal=.6, harm=.6, ring=.3, cdepth=.8, crate=1.0, cchaos=.8, corbit=-.5),
  P("Cosmic Strings", type=2, volume=-1, release=2.6, layer=.9, chorus=.3, reverb=.65, size=.97, width=1, attack=.4, shimmer=.3, cdepth=.7, crate=.15, corbit=.2),
  P("Haunted Signal", type=4, tone=-.3, release=1.2, layer=.3, chorus=.4, reverb=.45, size=.45, width=.8, hum=.4, wow=.5, crush=.35, vinyl=.3, cdepth=.85, crate=1.4, cchaos=1.0, cvar=.4),
  P("Lunar Tide", type=0, tone=-.2, release=2.5, layer=.6, chorus=.6, reverb=.7, size=.98, width=1, shimmer=.55, air=.25, cdepth=.75, crate=.07, corbit=.6),
  P("Chaos Crystal", type=1, volume=-1, tone=.3, release=1.6, layer=.3, chorus=.5, reverb=.6, width=1, octave=1, metal=.75, harm=.6, ring=.2, dmix=.3, dtime=2, cdepth=1.0, crate=.9, cchaos=.9, cvar=.4),
 ],
}

def fmt(v):
    v = float(v)
    return str(int(v)) if v.is_integer() else repr(round(v, 4))

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# equilibrage du volume entre presets (mesure sur le moteur)
LEVELS = json.load(open(os.path.join(root_dir, "tools", "levels.json"), encoding="utf-8"))
pack = os.path.join(root_dir, "HomeKey Preset")
shutil.rmtree(pack, ignore_errors=True)
count = 0
for folder, presets in PACK.items():
    os.makedirs(os.path.join(pack, folder), exist_ok=True)
    for name, kw in presets:
        vals = dict(D); vals.update(kw)
        assert set(kw) <= set(D), (name, set(kw) - set(D))
        vals.update(stars(vals["type"], name))
        vals["volume"] = round(max(-24.0, min(6.0, vals["volume"] + LEVELS.get(name, 0.0))), 1)
        params = "\n".join(f'  <PARAM id="{k}" value="{fmt(v)}"/>' for k, v in sorted(vals.items()))
        xml = f'<?xml version="1.0" encoding="UTF-8"?>\n\n<HomeKeysState presetName={quoteattr(name)}>\n{params}\n</HomeKeysState>\n'
        with open(os.path.join(pack, folder, name + ".hkpreset"), "w", encoding="utf-8") as f:
            f.write(xml)
        count += 1

with open(os.path.join(pack, "LISEZ-MOI.txt"), "w", encoding="utf-8") as f:
    f.write("HOMEKEY PRESET - pack officiel HomeKey I v3\n"
            "Ce pack s'installe TOUT SEUL au premier lancement de HomeKey I.\n"
            "Pour ajouter d'autres presets : bouton IMPORT dans HomeKey I.\n"
            "Astuce : charge un preset puis clique RANDOM SON avec MUTATION basse\n"
            "(0.2 - 0.4) pour obtenir des variations a chaque clic.\n")

os.makedirs(os.path.join(root_dir, "Resources"), exist_ok=True)
zpath = os.path.join(root_dir, "Resources", "HomeKeyPresets.zip")
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    for dp, _, files in os.walk(pack):
        for fn in sorted(files):
            full = os.path.join(dp, fn)
            z.write(full, os.path.relpath(full, root_dir))
print(count, "presets ->", pack)
