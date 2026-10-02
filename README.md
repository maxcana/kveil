# pdboffsetdownloader.py usage

if you are missing offsets, use `python pdboffsetdownloader.py <pdb_name> <guid> <age>`, hardcode them, and rebuild. it will tell you all these values in the error.

you might need to fix some dependency issues: `pip install setuptools && pip install "construct==2.10.70" && pip install pdbparse --no-build-isolation --no-deps && pip install requests`
