#include <utility>

#ifndef COMMONSTRUCTS_H
#define COMMONSTRUCTS_H

#include <complex>
#include "clwrapper.h"
#include <valarray>


struct Constants
{
    static const double Pi;
    // electron mass (kg)
    static const double eMass;
    // electron mass (keV)
    static const double eMassEnergy;
    // electron charge (C)
    static const double eCharge;
    // Planck's constant (Js)
    static const double h;
    // speed of light (m/s)
    static const double c;
    // Bohr radius (m)
    static const double a0;
    // Bohr radius (Angstrom)
    static const double a0A;
};

template<class T>
class Image
{
public:
    /// Default constructer
    Image() : width(0), height(0), depth(0), pad_t(0), pad_l(0), pad_b(0), pad_r(0) {}

    /// Constructor for an empty image (where you want to add the data later)
    Image(unsigned int w, unsigned int h, unsigned int d = 1, unsigned int pt = 0, unsigned int pl = 0, unsigned int pb = 0, unsigned int pr = 0) {
        width = w;
        height = h;
        depth = d;

        data.resize(d);

        for(auto& slice: data)
            slice.resize(w*h);

        pad_t = pt;
        pad_l = pl;
        pad_b = pb;
        pad_r = pr;
    }

    /// Constructor for a single image
    Image(std::vector<T> image, unsigned int w, unsigned int h, unsigned int pt = 0, unsigned int pl = 0, unsigned int pb = 0, unsigned int pr = 0) {
        data.push_back(image);
        width = w;
        height = h;
        depth = 1;

        pad_t = pt;
        pad_l = pl;
        pad_b = pb;
        pad_r = pr;
    }

    /// Constructor for a stack of images
    Image(std::vector<std::vector<T>> image, unsigned int w, unsigned int h, unsigned int pt = 0, unsigned int pl = 0, unsigned int pb = 0, unsigned int pr = 0) : data(image),
                                                                                                                                                                  width(w), height(h), depth(image.size()),
                                                                                                                                                                  pad_t(pt), pad_l(pl),
                                                                                                                                                                  pad_b(pb), pad_r(pr) {}

    Image(const Image<T>& rhs) : width(rhs.width), height(rhs.height), depth(rhs.depth), data(rhs.data),
                                                                                         pad_t(rhs.pad_t),
                                                                                         pad_l(rhs.pad_l),
                                                                                         pad_b(rhs.pad_b),
                                                                                         pad_r(rhs.pad_r) {}

    Image<T>& operator=(const Image<T>& rhs) {
        width = rhs.width;
        height = rhs.height;
        depth = rhs.depth;
        data = rhs.data;
        pad_t = rhs.pad_t;
        pad_l = rhs.pad_l;
        pad_b = rhs.pad_b;
        pad_r = rhs.pad_r;

        return *this;
    }

//    void addSlice(std::vector<T> im) {
//        if (im.size() != getSliceSize())
//            throw std::runtime_error("Append image to stack with incompatible sizes");
//        data.push_back(im);
//    }

    unsigned int getCroppedSliceSize() {return getCroppedWidth() * getCroppedHeight();}
    unsigned int getSliceSize(bool crop = false) {
        if (crop)
            return getCroppedSliceSize();
        else
            return width * height;
    }

    std::valarray<unsigned int> getDimensions() { return {getWidth(), getHeight(), getDepth()}; }
    std::valarray<unsigned int> getCroppedDimensions() { return {getCroppedWidth(), getCroppedHeight(), getDepth()}; }

    unsigned int getWidth(bool crop = false) {if(crop) return getCroppedWidth(); else return width;}
    unsigned int getHeight(bool crop = false) {if(crop) return getCroppedHeight(); else return height;}
    unsigned int getDepth() {return depth;}
    unsigned int getCroppedWidth() {return width - pad_l - pad_r;}
    unsigned int getCroppedHeight() {return height - pad_t - pad_b;}

    std::valarray<unsigned int> getPadding() { return {pad_t, pad_l, pad_b, pad_r}; }

    std::vector<T>& getSliceRef(unsigned int slice = 0) {
        return data[slice];
    }

    std::vector<T> getSlice(unsigned int slice = 0, bool crop = false) {
        if (crop)
            return getCroppedSlice(slice);
        return data[slice];
    }

    std::vector<T> getCroppedSlice(unsigned int slice = 0) {

        std::vector<T> out(getCroppedSliceSize());

        unsigned int counter = 0;
        for (int j = pad_b; j < getHeight()-pad_t; ++j)
            for (int i = pad_l; i < getWidth() - pad_r; ++i) {
                unsigned int index = j * getWidth() + i;
                out[counter] = data[slice][index];
                counter++;
            }

        return out;
    }

private:
    // bool is_complex; // this sets if the data is interleaved complex (i.e. real->img->real->imag etc...)
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int pad_t, pad_l, pad_b, pad_r;
    std::vector<std::vector<T>> data;
};

struct ComplexAberration {
    ComplexAberration() : Mag(0.0f), Ang(0.0f) {}
    ComplexAberration(double m, double a) : Mag(m), Ang(a) {}

    double Mag, Ang;

    std::complex<double> getComplex() {return std::polar(Mag, Ang);}
};

struct MicroscopeParameters {
    MicroscopeParameters() : C10(0.0f), C30(0.0f), C50(0.0f), Voltage(1.0f), Aperture(1.0f), Alpha(1.0f), Delta(1.0f),
                             BeamTilt(0.0), BeamAzimuth(0.0) {}

    // Defocus
    double C10;
    //Two-fold astigmatism
    ComplexAberration C12;
    //Coma
    ComplexAberration C21;
    //Three-fold astigmatism
    ComplexAberration C23;
    //Spherical
    double C30;

    ComplexAberration C32;

    ComplexAberration C34;

    ComplexAberration C41;

    ComplexAberration C43;

    ComplexAberration C45;
    //Fifth order spherical
    double C50;

    ComplexAberration C52;

    ComplexAberration C54;

    ComplexAberration C56;

    // Voltage (kV) == (kg m^2 C^-1 s^-2)
    double Voltage;
    // TEM: Objective aperture radius (mrad)
    // STEM: Condenser aperture radius (mrad)
    double Aperture;
    // Aperture smoothing radius (mrad)
    double ApertureSmoothing;

    // Beam tilt inclination (mrad)
    double BeamTilt;
    // Beam tilt azimuth (rad)
    double BeamAzimuth;

    //Convergence angle (mrad)
    double Alpha;
    //Defocus spread (Angstroms)
    double Delta;

    //Calculate wavelength (Angstroms)
    double Wavelength() {
        return Constants::h * Constants::c / std::sqrt( Constants::eCharge * (Voltage * 1000) * (2 * Constants::eMass * Constants::c*Constants::c + Constants::eCharge * ( Voltage * 1000 ) ) ) * 1e+10;
    }

    double Wavenumber() {
        return 1.0 / Wavelength();
    }

    // Interaction parameter (see Kirkland Eq. 5.6) (s^2 C m^-2 kg^-1 Angstrom^-1])
    double Sigma() {
        return (2 * Constants::Pi / (Wavelength() * (Voltage * 1000))) * (Constants::eMass*Constants::c*Constants::c + Constants::eCharge * (Voltage * 1000)) / (2 * Constants::eMass*Constants::c*Constants::c + Constants::eCharge * (Voltage * 1000));
    }

    std::valarray<double> Wavevector() {
        double k = Wavenumber();
        double k_x = k * std::sin(BeamTilt / 1000.0) * std::cos(BeamAzimuth);
        double k_y = k * std::sin(BeamTilt / 1000.0) * std::sin(BeamAzimuth);
        double k_z = k * std::cos(BeamTilt / 1000.0);

        return {k_x, k_y, k_z};
    }
};

struct StemDetector {
    StemDetector() : name("--"), inner(0.0), outer(1.0), xcentre(0.0), ycentre(0.0) {}
    StemDetector(std::string nm, double in, double out, double xc, double yc) : name(std::move(nm)),
            inner(in), outer(out), xcentre(xc), ycentre(yc) {}

    std::string name;

    double inner, outer, xcentre, ycentre;
};

struct SimulationArea {
    SimulationArea() : xStart(0.0), xFinish(10.0), yStart(0.0), yFinish(10.0), padding(0.0) {}

    SimulationArea(double xs, double xf, double ys, double yf, double pd = 0.f) : xStart(xs), xFinish(xf),
                                                                                  yStart(ys), yFinish(yf),
                                                                                  padding(pd) {}

    std::valarray<double> getRawLimitsX();
    std::valarray<double> getRawLimitsY();

    void setRawLimitsX(double start, double finish);
    void setRawLimitsY(double start, double finish);

    std::valarray<double> getCorrectedLimitsX();
    std::valarray<double> getCorrectedLimitsY();

    void setPadding(double pd) {padding = pd;}

protected:
    double xStart, xFinish, yStart, yFinish, padding;
};

// TODO: is this class necessary, can we not just use the sim area one with the start and end points the same?
struct CbedPosition {
    CbedPosition() : xPos(0.0), yPos(0.0), padding(0.0) {}
    CbedPosition(double _x, double _y, double _pd = 0.0) : xPos(_x), yPos(_y), padding(_pd) {}

    double getXPos() {return xPos;}
    double getYPos() {return yPos;}
    double getPadding() {return padding;}

    void setXPos(double xp) {xPos = xp;}
    void setYPos(double yp) {yPos = yp;}
    void setPaddding(double pd) {padding = pd;}

    void setPos(double xp, double yp) {
        xPos = xp;
        yPos = yp;
    }

    SimulationArea getSimArea() {
        // pad equally on both sides
        return SimulationArea(xPos, xPos, yPos, yPos, padding);
    }

private:
    double xPos, yPos, padding;
};

struct StemArea : public SimulationArea {
    static const double DefaultScale;

    StemArea() : SimulationArea(), xPixels(64), yPixels(64) {}
    StemArea(double xs, double xf, double ys, double yf, unsigned int xp, unsigned int yp, double pd = 0.0) : SimulationArea(xs, xf, ys, yf, pd),
                                                                                 xPixels(xp), yPixels(yp) {}

    bool setPxRangeX(double start, double finish, int px);
    bool setPxRangeY(double start, double finish, int px);

    void forcePxRangeX(double start, double finish, int px);
    void forcePxRangeY(double start, double finish, int px);

    void setPixelsX(unsigned int px) {xPixels = px;}
    void setPixelsY(unsigned int py) {yPixels = py;}

    double getStemPixelScaleX() { return (xFinish - xStart) / xPixels;}
    double getStemPixelScaleY() { return (yFinish - yStart) / yPixels;}

    unsigned int getPixelsX() {return xPixels;}
    unsigned int getPixelsY() {return yPixels;}
    double getPadding() {return padding;}

    double getScaleX();
    double getScaleY();

    unsigned int getNumPixels() {return xPixels * yPixels;}

    SimulationArea getPixelSimArea(int pixel) {
        // convert pixel id to x, y position
        unsigned int x_px = pixel % xPixels;
        unsigned int y_px = (unsigned int) std::floor(pixel / xPixels);

        float xPos = getRawLimitsX()[0] + getScaleX() * x_px;
        float yPos = getRawLimitsY()[0] + getScaleY() * y_px;
        // pad equally on both sides
        return SimulationArea(xPos, xPos, yPos, yPos, padding);
    }

private:
    unsigned int xPixels, yPixels;

};
#endif //COMMONSTRUCTS_H
