"""Prepare les banques de sons libres integrees a HomeKey I.

Sources (a telecharger a part) :
  - Salamander Grand Piano V3, Alexander Holm, CC-BY 3.0
    https://github.com/sfzinstruments/SalamanderGrandPiano
  - Versilian Community Sample Library (VCSL), CC0
    https://github.com/sgossner/VCSL

Modifications : notes raccourcies avec fondu, 4 couches de velocite (v4, v8, v12, v16),
reechantillonnage 44.1 kHz, conversion Ogg Vorbis.

Usage : python tools/make_samples.py <dossier Salamander> <dossier VCSL>
"""
import os, re, subprocess, sys, tempfile, zipfile

SAL, VCSL = sys.argv[1], sys.argv[2]
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Resources", "Samples.zip")

NOTE = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}
LAYERS = [4, 8, 12, 16]

TEXTURES = [  # (fichier VCSL, note de base MIDI, duree max s)
    ("Idiophones/Struck Idiophones/Gong 1/gong_mf.wav", 48, 12.0),
    ("Idiophones/Struck Idiophones/Suspended Cymbal 1/susCymb1_bow_17.wav", 60, 12.0),
    ("Idiophones/Struck Idiophones/Vibraphone/Bowed/Vibes_bowed_E3_rr1_Main.wav", 52, 12.0),
    ("Idiophones/Struck Idiophones/Tubular Bells 2/TB_hit_C4_v4_1.wav", 60, 10.0),
    ("Idiophones/Struck Idiophones/Anvil/Anvil_Hit2_v3_rr1_Mid.wav", 60, 2.0),
    ("Idiophones/Struck Idiophones/Brake Drum/BrakeDrum2_Bowed_rr1_Mid.wav", 60, 5.8),
    ("Idiophones/Struck Idiophones/Hand Bells, Nepalese/HB_1.wav", 72, 3.0),
]

def encode(src, dst, seconds, normalize=False):
    fade = min(0.9, seconds * 0.25)
    af = f"atrim=0:{seconds},afade=t=out:st={seconds - fade}:d={fade}"
    if normalize:
        af += ",loudnorm=I=-16:TP=-1.5:LRA=11"
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", src, "-af", af, "-ar", "44100",
                    "-ac", "2", "-c:a", "libvorbis", "-q:a", "5", dst], check=True)

with tempfile.TemporaryDirectory() as tmp, zipfile.ZipFile(OUT, "w", zipfile.ZIP_STORED) as z:
    n = 0
    for fn in sorted(os.listdir(os.path.join(SAL, "Samples"))):
        m = re.fullmatch(r"([A-G]#?)(\d)v(\d+)\.flac", fn)
        if not m or int(m.group(3)) not in LAYERS:
            continue
        midi = 12 * (int(m.group(2)) + 1) + NOTE[m.group(1)]
        layer = LAYERS.index(int(m.group(3)))
        seconds = max(3.5, min(10.0, 10.0 - (midi - 21) * 0.09))
        name = f"p_{midi}_{layer}.ogg"
        encode(os.path.join(SAL, "Samples", fn), os.path.join(tmp, name), seconds)
        z.write(os.path.join(tmp, name), name)
        n += 1
    for i, (rel, root, seconds) in enumerate(TEXTURES):
        name = f"t_{i}_{root}.ogg"
        encode(os.path.join(VCSL, rel), os.path.join(tmp, name), seconds, normalize=True)
        z.write(os.path.join(tmp, name), name)
        n += 1
print(n, "fichiers ->", OUT, round(os.path.getsize(OUT) / 1e6, 1), "MB")
