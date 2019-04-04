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
    double getPosX() const {
        return position[0];
    }

    double getPosY() const {
        return position[1];
    }

    double getPosZ() const {
        return position[2];
    }

    void setPosX(double x){
        position[0] = x;
        defaultPosition = false;
    }

    void setPosY(double y){
        position[1] = y;
        defaultPosition = false;
    }

    void setPosZ(double z){
        position[2] = z;
        defaultPosition = false;
    }

    void setPos(double x, double y, double z){
        setPosX(x);
        setPosY(y);
        setPosZ(z);
    }

    void setPos(const std::array<double, 3> &position) {
        Camera::position = position;
        defaultPosition = false;
    }

    std::array<double, 3> getPos() const{
        return position;
    }

    //Focus

    double getFocX() const {
        return focalPoint[0];
    }

    double getFocY() const {
        return focalPoint[1];
    }

    double getFocZ() const {
        return focalPoint[2];
    }

    void setFocX(double x){
        focalPoint[0] = x;
        defaultFocalPoint = false;
    }

    void setFocY(double y){
        focalPoint[1] = y;
        defaultFocalPoint = false;
    }

    void setFocZ(double z){
        focalPoint[2] = z;
        defaultFocalPoint = false;
    }

    void setFoc(double x, double y, double z){
        setFocX(x);
        setFocY(y);
        setFocZ(z);
    }

    void setFoc(const std::array<double, 3> &focalPoint) {
        Camera::focalPoint = focalPoint;
        defaultFocalPoint = false;
    }

    std::array<double, 3> getFoc() const{
        return focalPoint;
    }

    //ViewUP

    double getViewX() const{
        return viewUp[0];
    }

    double getViewY() const{
        return viewUp[1];
    }

    double getViewZ() const{
        return viewUp[2];
    }

    void setViewX(double x){
        viewUp[0] = x;
        defaultViewUp = false;
    }

    void setViewY(double y){
        viewUp[1] = y;
        defaultViewUp = false;
    }

    void setViewZ(double z){
        viewUp[2] = z;
        defaultViewUp = false;
    }

    void setView(double x, double y, double z){
        setViewX(x);
        setViewY(y);
        setViewZ(z);
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

    void setDefaultPosition(bool defaultPosition) {
        Camera::defaultPosition = defaultPosition;
    }

    void setDefaultFocalPoint(bool defaultFocalPoint) {
        Camera::defaultFocalPoint = defaultFocalPoint;
    }

    void setDefaultViewUp(bool defaultViewUp) {
        Camera::defaultViewUp = defaultViewUp;
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



inline double countStep(double a, double b, unsigned long step, unsigned long totalSteps){
    return a + (((b - a) * step) / totalSteps);
}

inline double vecSize(const std::array<double, 3>& a, const std::array<double, 3>& b){
    return std::sqrt(((a[0] - b[0]) * (a[0] - b[0])) +
                     ((a[1] - b[1]) * (a[1] - b[1])) +
                     ((a[2] - b[2]) * (a[2] - b[2])));
}

inline void setPosition(const Camera& cameraStart, const Camera& cameraEnd, Camera& res,
                 unsigned long step, unsigned long totalSteps){
    res.setPosX(countStep(cameraStart.getPosX(), cameraEnd.getPosX(), step, totalSteps));
    res.setPosY(countStep(cameraStart.getPosY(), cameraEnd.getPosY(), step, totalSteps));
    res.setPosZ(countStep(cameraStart.getPosZ(), cameraEnd.getPosZ(), step, totalSteps));
}

inline void setView(const Camera& cameraStart, const Camera& cameraEnd, Camera& res,
             unsigned long step, unsigned long totalSteps){
    res.setViewX(countStep(cameraStart.getViewX(), cameraEnd.getViewX(), step, totalSteps));
    res.setViewY(countStep(cameraStart.getViewY(), cameraEnd.getViewY(), step, totalSteps));
    res.setViewZ(countStep(cameraStart.getViewZ(), cameraEnd.getViewZ(), step, totalSteps));
}

inline void setFocus(const Camera& cameraStart, const Camera& cameraEnd, Camera& res,
              unsigned long step, unsigned long totalSteps){
    res.setFocX(countStep(cameraStart.getFocX(), cameraEnd.getFocX(), step, totalSteps));
    res.setFocY(countStep(cameraStart.getFocY(), cameraEnd.getFocY(), step, totalSteps));
    res.setFocZ(countStep(cameraStart.getFocZ(), cameraEnd.getFocZ(), step, totalSteps));
}

inline std::array<double, 3> countLinearPosition(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    res[0] = countStep(cameraStart.getPosX(), cameraEnd.getPosX(), step, totalSteps);
    res[1] = countStep(cameraStart.getPosY(), cameraEnd.getPosY(), step, totalSteps);
    res[2] = countStep(cameraStart.getPosZ(), cameraEnd.getPosZ(), step, totalSteps);
    return res;
}

inline std::array<double, 3> countLinearFocus(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    res[0] = countStep(cameraStart.getFocX(), cameraEnd.getFocX(), step, totalSteps);
    res[1] = countStep(cameraStart.getFocY(), cameraEnd.getFocY(), step, totalSteps);
    res[2] = countStep(cameraStart.getFocZ(), cameraEnd.getFocZ(), step, totalSteps);
    return res;
}

inline std::array<double, 3> countLinearView(const Camera& cameraStart, const Camera& cameraEnd,
        unsigned long step, unsigned long totalSteps){
    std::array<double, 3> res{};
    res[0] = countStep(cameraStart.getViewX(), cameraEnd.getViewX(), step, totalSteps);
    res[1] = countStep(cameraStart.getViewY(), cameraEnd.getViewY(), step, totalSteps);
    res[2] = countStep(cameraStart.getViewZ(), cameraEnd.getViewZ(), step, totalSteps);
    return res;
}

inline double countDistance(const Camera& cameraStart, const Camera& cameraEnd, unsigned long step,
                     unsigned long totalSteps){
    double d1 = vecSize(cameraStart.getPos(), cameraStart.getFoc());
    double d2 = vecSize(cameraEnd.getPos(), cameraEnd.getFoc());
    return countStep(d1, d2, step, totalSteps);
}

inline Camera countCameraMove(const Camera& cameraStart, const Camera& cameraEnd,
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
