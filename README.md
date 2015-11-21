# k2-pca

*Applying PCA to K2 lightcurves*

## Procedure

First we want to combine all of the flux measurements into a single
file, for more efficient data use.  For this purpose, the file
`src/extract.cpp` gets built into the `bin/extract` program. Then supply
the names of K2 files (with detrended flux), and `-o/--output` argument.
