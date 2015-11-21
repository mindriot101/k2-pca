#include <iostream>
#include <cassert>
#include <cmath>

#include <tclap/CmdLine.h>
#include <fitsio.h>

#define fits_check(x) (x); if (status) { fits_report_error(stderr, status); exit(status); }
#define fits_checkp(x) (x); if (*status) { fits_report_error(stderr, *status); exit(*status); }

using namespace std;

const double NULL_VALUE = NAN;

long get_nimages(const string &filename) {
    fitsfile *fptr;
    int status = 0;
    
    fits_check(fits_open_file(&fptr, filename.c_str(), READONLY, &status));

    int hdutype = 0;
    fits_check(fits_movabs_hdu(fptr, 2, &hdutype, &status));
    assert(BINARY_TBL == hdutype);

    long nrows = 0;
    fits_check(fits_get_num_rows(fptr, &nrows, &status));

    fits_check(fits_close_file(fptr, &status));
    return nrows;
}

void compute_image_dimensions(const vector<string> &files, long *nobjects, long *nimages) {
    *nobjects = files.size();
    vector<long> sizes;
    transform(files.begin(), files.end(), back_inserter(sizes), get_nimages);
    *nimages = *max_element(sizes.begin(), sizes.end());
    cout << endl;
}

vector<double> get_column(const string &filename, const string &column, int datatype, int *status) {
    fitsfile *fptr;
    long nimages = get_nimages(filename);
    vector<double> out(nimages);

    fits_checkp(fits_open_file(&fptr, filename.c_str(), READONLY, status));
    fits_checkp(fits_movabs_hdu(fptr, 2, NULL, status));

    int colnum = 0;
    fits_checkp(fits_get_colnum(fptr, CASEINSEN, const_cast<char*>(column.c_str()), &colnum, status));
    assert(0 != colnum);
    cout << column << " column number: " << colnum << endl;

    fits_checkp(fits_read_col(fptr, datatype, colnum, 1, 1, nimages, NULL, &out[0], NULL, status));
    fits_checkp(fits_close_file(fptr, status));

    return out;
}

void create_image_hdu(const string &hdu_name, const string &column, fitsfile *fptr, int datatype, const vector<string> &files, long nobjects, long nimages, int *status) {
    cout << "Creating " << hdu_name << " hdu, from column " << column << endl;
    long naxes[] = {nimages, nobjects};

    switch (datatype) {
        case TDOUBLE:
            fits_checkp(fits_create_img(fptr, DOUBLE_IMG, 2, naxes, status));
            break;
        case TINT:
            fits_checkp(fits_create_img(fptr, LONG_IMG, 2, naxes, status));
            break;
        default:
            cerr << "Unsupported data type: " << datatype << endl;
            exit(-1);
            break;
    }

    /* Check that we've changed hdu */
    int hdunum = 0;
    fits_checkp(fits_get_hdu_num(fptr, &hdunum));
    assert(1 != hdunum);

    cout << "Writing hdu name" << endl;
    char *extname = const_cast<char*>(hdu_name.c_str());
    fits_checkp(fits_update_key(fptr, TSTRING, "EXTNAME", extname, NULL, status));

    for (int i=0; i<nobjects; i++) {
        auto filename = files[i];
        cout << "Updating from file " << filename << endl;
        auto data = get_column(filename, column, datatype, status);
        for (int j=data.size(); j<nimages; j++) {
            data.push_back(NULL_VALUE);
        }
        long fpixel[] = {1, i + 1};
        long lpixel[] = {nimages, i + 1};
        fits_checkp(fits_write_subset(fptr, datatype, fpixel, lpixel, &data[0], status));
    }
}

fitsfile *create_and_clobber(const string &filename) {
    int status = 0;
    fitsfile *fptr = nullptr;

    fits_create_file(&fptr, filename.c_str(), &status);
    if (FILE_NOT_CREATED == status) {
        cerr << "File " << filename << " exists, overwriting" << endl;
        remove(filename.c_str());
        status = 0;
        return create_and_clobber(filename);
    } else if (0 != status) {
        fits_report_error(stderr, status);
        exit(status);
    }

    return fptr;
}

void combine_files(const vector<string> &files, const string &output) {
    long nobjects = 0, nimages = 0;
    fitsfile *fptr;
    int status = 0;
    compute_image_dimensions(files, &nobjects, &nimages);
    cout << "Image dimensions: " << nimages << " images, " << nobjects << " objects" << endl;

    fptr = create_and_clobber(output);
    assert(nullptr != fptr);

    /* Create the primary hdu */
    cout << "Creating primary hdu" << endl;
    fits_check(fits_create_img(fptr, BYTE_IMG, 0, NULL, &status));

    /* Write the image HDUS */
    fits_check(create_image_hdu("HJD", "TIME", fptr, TDOUBLE, files, nobjects, nimages, &status));
    fits_check(create_image_hdu("FLUX", "DETFLUX", fptr, TDOUBLE, files, nobjects, nimages, &status));
    fits_check(create_image_hdu("FLUXERR", "DETFLUX_ERR", fptr, TDOUBLE, files, nobjects, nimages, &status));
    fits_check(create_image_hdu("CCDX", "CENT_COL", fptr, TDOUBLE, files, nobjects, nimages, &status));
    fits_check(create_image_hdu("CCDY", "CENT_ROW", fptr, TDOUBLE, files, nobjects, nimages, &status));
    fits_check(create_image_hdu("QUALITY", "QUALITY", fptr, TINT, files, nobjects, nimages, &status));


    fits_check(fits_close_file(fptr, &status));
}

int main(int argc, const char *argv[]) {
    try {
        TCLAP::CmdLine cmd("extract", ' ', "0.0.1");
        TCLAP::UnlabeledMultiArg<string> filesArg("files", "files to combine", true, "filename", cmd);
        TCLAP::ValueArg<string> outputArg("o", "output", "output filename", true, "", "filename", cmd);
        cmd.parse(argc, argv);

        combine_files(filesArg.getValue(), outputArg.getValue());
    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }
    return 0;
}
