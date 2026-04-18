#!/bin/sh
# test_apl_install - test APL font installation

echo "Removing user-installed APL385 font..."

rm -f $HOME/.local/share/fonts/Apl385.ttf
rm -f $HOME/Library/Fonts/Apl385.ttf

echo
echo "Checking that font is not found..."

if [ "`uname`" != "Darwin" ]; then
  fc-cache -f $HOME/.local/share/fonts

  echo
  echo "fc-match result:"
  fc-match "APL385 Unicode"

  echo
  echo "All matching fonts:"
  FONTS=`fc-list | grep -i apl385`

  if [ -z "$FONTS" ]; then
    echo "No APL385 font found"
  else
    echo "$FONTS"
    echo
    echo "Warning: APL385 font still present (possibly system-wide install)"
  fi
else
  echo
  echo "Installed font files:"
  ls $HOME/Library/Fonts/Apl385.ttf 2>/dev/null || echo "No APL385 font found"
fi

echo
echo "Reinstalling font..."

./install_apl

echo
echo "Checking that font is installed..."

if [ "`uname`" != "Darwin" ]; then
  echo
  echo "fc-match result:"
  fc-match "APL385 Unicode"

  echo
  echo "All matching fonts:"
  fc-list | grep -i apl385
else
  echo
  echo "Installed font files:"
  ls $HOME/Library/Fonts/Apl385.ttf
fi

echo
echo "Done. You can now test with:"
echo "tek4010 -APL -noexit ./apltest"