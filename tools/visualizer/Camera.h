//
// Created by maki on 10.3.19.
//

#ifndef ROFI_CAMERA_H
#define ROFI_CAMERA_H

#include <array>
#include <legacy/configuration/Configuration.h>

namespace {
    using namespace rofi::configuration::matrices;

/**
 * This class have parameters for vtkCamera.
 * Default values are from documentation. *
 */
class Camera{
    std::array<double, 3> position = {0, 0, 1};
    std::array<double, 3> focalPoint = {0, 0, 0};
    std::array<double, 3> viewUp = {0, 1, 0};
    bool defaultPosition = true;
    bool defaultFocalPoint = true;
    bool defaultViewUp = true;

public:
    void setPos(double x, double y, double z){
        position[0] = x;
        position[1] = y;
        position[2] = z;
        defaultPosition = false;
    }

    void setPos(const std::array<double, 3> &position) {
        Camera::position = position;
        defaultPosition = false;
    }

    std::array<double, 3> getPos() const{
        return position;
    }

    void setFoc(double x, double y, double z){
        focalPoint[0] = x;
        focalPoint[1] = y;
        focalPoint[2] = z;
        defaultFocalPoint = false;
    }

    void setFoc(const std::array<double, 3> &focalPoint) {
        Camera::focalPoint = focalPoint;
        defaultFocalPoint = false;
    }

    std::array<double, 3> getFoc() const{
        return focalPoint;
    }

    void setView(double x, double y, double z){
        viewUp[0] = x;
        viewUp[1] = y;
        viewUp[2] = z;
        defaultViewUp = false;
    }

    void setView(const std::array<double, 3> &viewUp) {
        Camera::viewUp = viewUp;
        defaultViewUp = false;
    }

    std::array<double, 3> getView() const{
        return viewUp;
    }


    void reset(){
        setPos(0, 0, 1);
        setFoc(0, 0, 0);
        setView(0, 1, 0);
        defaultPosition = true;
        defaultFocalPoint = true;
        defaultViewUp = true;
    }

    bool defaultPos() const{
        return defaultPosition;
    }

    bool defaultFoc() const{
        return defaultFocalPoint;
    }

    bool defaultView() const{
        return defaultViewUp;
    }

    /**
     * This function overwrites default camera params by new values
     * depending on the mass center of a configuration.
     *
     * @param massCenter mass center of the configuration
     */
    void setCameraMassCenter(Vector massCenter){
        if (defaultPos()) {
            setPos(massCenter(0), massCenter(1) - 6, massCenter(2));
            defaultPosition = false;
        }
        if (defaultFoc()) {
            setFoc(massCenter(0), massCenter(1), massCenter(2));
            defaultFocalPoint = false;
        }
        if (defaultView()) {
            setView(0, 0, 1);
            defaultViewUp = false;
        }
    }

    bool operator==(const Camera& other) const {
        return this->getFoc() == other.getFoc() &&
                this->getView() == other.getView() &&
                this->getPos() == other.getPos();
    }
};


/**
 * This function linearly interpolates between value a and b.
 *
 * @param a value at the beginning
 * @param b value at the end
 * @param step currently counted step number
 * @param totalSteps total number of steps
 * @return interpolated value
 */
inline double countSteps(double a, double b, unsigned long step, unsigned long totalSteps){
    if (totalSteps == 0){
        return a;
    }
    // It is safe to assume that number of steps will fit into a double
    return a + (((b - a) * double(step)) / double(totalSteps));
}

/**
 * This function counts distance of two points in space.
 *
 * @param a coordinates of one point
 * @param b coordinates of the second point
 * @return distance of two points
 */
inline double vecSize(const std::array<double, 3>& a, const std::array<double, 3>& b){
    return std::sqrt(((a[0] - b[0]) * (a[0] - b[0])) +
                     ((a[1] - b[1]) * (a[1] - b[1])) +
                     ((a[2] - b[2]) * (a[2] - b[2])));
}

/**
 * This function linearly interpolates position of camera.
 *
 * @param cameraStart camera parameters at the beginning
 * @param cameraEnd camera parameters at the end
 * @param step currently counted step number
 * @param totalSteps total number of steps
 * @return linearly interpolated camera position coordinates
 */
inline std::array<double, 3> countLinearPosition(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    for (int i = 0; i < 3; i++){
        res[i] = countSteps(cameraStart.getPos()[i], cameraEnd.getPos()[i], step, totalSteps);
    }
    return res;
}

/**
 * This function linearly interpolates focal point of camera.
 *
 * @param cameraStart camera parameters at the beginning
 * @param cameraEnd camera parameters at the end
 * @param step currently counted step number
 * @param totalSteps total number of steps
 * @return linearly interpolated camera focal point coordinates
 */
inline std::array<double, 3> countLinearFocus(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    for (int i = 0; i < 3; i++){
        res[i] = countSteps(cameraStart.getFoc()[i], cameraEnd.getFoc()[i], step, totalSteps);
    }
    return res;
}

/**
 * This function linearly interpolates viewUp vector of camera.
 *
 * @param cameraStart camera parameters at the beginning
 * @param cameraEnd camera parameters at the end
 * @param step currently counted step number
 * @param totalSteps total number of steps
 * @return linearly interpolated camera viewUp vector coordinates
 */
inline std::array<double, 3> countLinearView(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    for (int i = 0; i < 3; i++){
        res[i] = countSteps(cameraStart.getView()[i], cameraEnd.getView()[i], step, totalSteps);
    }
    return res;
}

/**
 * This function linearly interpolates distance of focal point and position of camera.
 *
 * @param cameraStart camera parameters at the beginning
 * @param cameraEnd camera parameters at the end
 * @param step currently counted step number
 * @param totalSteps total number of steps
 * @return linearly interpolated distance of focal point and position
 */
inline double countDistance(const Camera& cameraStart, const Camera& cameraEnd, unsigned long step,
                     unsigned long totalSteps){
    double d1 = vecSize(cameraStart.getPos(), cameraStart.getFoc());
    double d2 = vecSize(cameraEnd.getPos(), cameraEnd.getFoc());
    return countSteps(d1, d2, step, totalSteps);
}

/**
 * This function interpolates camera parameters between cameraStart and cameraEnd.
 *
 * @param cameraStart camera parameters at the beginning
 * @param cameraEnd camera parameters at the end
 * @param step currently counted step number
 * @param totalSteps total number of steps
 * @return interpolated camera parameters
 */
inline Camera interpolateCamera(const Camera& cameraStart, const Camera& cameraEnd,
                       unsigned long step, unsigned long totalSteps){
    if (cameraStart == cameraEnd){
        return cameraStart;
    }
    Camera res;
    std::array<double, 3> linearPosition = countLinearPosition(cameraStart, cameraEnd, step, totalSteps);
    std::array<double, 3> linearFocus = countLinearFocus(cameraStart, cameraEnd, step, totalSteps);
    std::array<double, 3> linearView = countLinearView(cameraStart, cameraEnd, step, totalSteps);
    res.setFoc(linearFocus);
    res.setView(linearView);

    double wantedDistance = countDistance(cameraStart, cameraEnd, step, totalSteps);
    double realDistance = vecSize(linearPosition, linearFocus);
    double coefficient = wantedDistance / realDistance;

    double x = coefficient * (linearPosition[0] - linearFocus[0]) + linearFocus[0];
    double y = coefficient * (linearPosition[1] - linearFocus[1]) + linearFocus[1];
    double z = coefficient * (linearPosition[2] - linearFocus[2]) + linearFocus[2];

    res.setPos(x, y, z);

    return res;
}

namespace IO {

inline void readCameraSettings(std::istream &input, Camera &cameraStart, Camera &cameraEnd, bool &cameraMove)
{
    std::string line;
    while (getline(input, line)) {
        if (line[0] != 'C' && !line.empty()) {     //not for camera settings
            throw std::runtime_error("Expected camera settings (CP, CPM, CF, CFM, CV, CVM), got " + line + ".");
        }
        std::stringstream str(line);
        std::string type;
        str >> type;
        double xs, ys, zs, xe, ye, ze;
        if (type == "CP") {          //camera position
            str >> xs >> ys >> zs;
            cameraStart.setPos(xs, ys, zs);
            cameraEnd.setPos(xs, ys, zs);
        } else if (type == "CV") {   //camera viewUp
            str >> xs >> ys >> zs;
            cameraStart.setView(xs, ys, zs);
            cameraEnd.setView(xs, ys, zs);
        } else if (type == "CF") {   //camera focal point
            str >> xs >> ys >> zs;
            cameraStart.setFoc(xs, ys, zs);
            cameraEnd.setFoc(xs, ys, zs);
        } else if (type == "CPM") {  //camera position move
            str >> xs >> xe >> ys >> ye >> zs >> ze;
            cameraStart.setPos(xs, ys, zs);
            cameraEnd.setPos(xe, ye, ze);
            cameraMove = true;
        } else if (type == "CVM") {  //camera viewUp move
            str >> xs >> xe >> ys >> ye >> zs >> ze;
            cameraStart.setView(xs, ys, zs);
            cameraEnd.setView(xe, ye, ze);
            cameraMove = true;
        } else if (type == "CFM") {  //camera focal point move
            str >> xs >> xe >> ys >> ye >> zs >> ze;
            cameraStart.setFoc(xs, ys, zs);
            cameraEnd.setFoc(xe, ye, ze);
            cameraMove = true;
        }
        else if (!type.empty()){
            throw std::runtime_error("Expected camera settings (CP, CPM, CF, CFM, CV, CVM), got " + type + ".");
        }
    }
}

} // namespace IO

} // namespace

#endif //ROFI_CAMERA_H
