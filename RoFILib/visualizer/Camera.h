//
// Created by maki on 10.3.19.
//

#ifndef ROFI_CAMERA_H
#define ROFI_CAMERA_H

#include <array>

/**
 * Parameters for vtkCamera
 * default values from documentation
 *
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

    void setCameraMassCenter(Vector massCenter){
        if (defaultPos()) {
            setPos(massCenter(0), massCenter(1) - 6, massCenter(2));
            defaultPosition = true;
        }
        if (defaultFoc()) {
            setFoc(massCenter(0), massCenter(1), massCenter(2));
            defaultFocalPoint = true;
        }
        if (defaultView()) {
            setView(0, 0, 1);
            defaultViewUp = true;
        }
    }

    bool operator==(const Camera& other) const {
        return this->getFoc() == other.getFoc() &&
                this->getView() == other.getView() &&
                this->getPos() == other.getPos();
    }
};



inline double countSteps(double a, double b, unsigned long step, unsigned long totalSteps){
    if (totalSteps == 0){
        return a;
    }
    return a + (((b - a) * step) / totalSteps);
}

inline double vecSize(const std::array<double, 3>& a, const std::array<double, 3>& b){
    return std::sqrt(((a[0] - b[0]) * (a[0] - b[0])) +
                     ((a[1] - b[1]) * (a[1] - b[1])) +
                     ((a[2] - b[2]) * (a[2] - b[2])));
}

inline std::array<double, 3> countLinearPosition(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    for (int i = 0; i < 3; i++){
        res[i] = countSteps(cameraStart.getPos()[i], cameraEnd.getPos()[i], step, totalSteps);
    }
    return res;
}

inline std::array<double, 3> countLinearFocus(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    for (int i = 0; i < 3; i++){
        res[i] = countSteps(cameraStart.getFoc()[i], cameraEnd.getFoc()[i], step, totalSteps);
    }
    return res;
}

inline std::array<double, 3> countLinearView(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    for (int i = 0; i < 3; i++){
        res[i] = countSteps(cameraStart.getView()[i], cameraEnd.getView()[i], step, totalSteps);
    }
    return res;
}

inline double countDistance(const Camera& cameraStart, const Camera& cameraEnd, unsigned long step,
                     unsigned long totalSteps){
    double d1 = vecSize(cameraStart.getPos(), cameraStart.getFoc());
    double d2 = vecSize(cameraEnd.getPos(), cameraEnd.getFoc());
    return countSteps(d1, d2, step, totalSteps);
}

inline Camera interpolateCamera(const Camera& cameraStart, const Camera& cameraEnd,
                       unsigned long step, unsigned long totalSteps){
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

#endif //ROFI_CAMERA_H
