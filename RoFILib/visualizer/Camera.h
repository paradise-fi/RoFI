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
};

#endif //ROFI_CAMERA_H
