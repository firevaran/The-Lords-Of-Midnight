find . -type f -name '.DS*' -delete

rm -r Builds/mac-citadel
mkdir Builds/mac-citadel
axmol build -configOnly -p osx -a x64 -xc '-DTME=CITADEL,-BBuilds/mac-citadel'
