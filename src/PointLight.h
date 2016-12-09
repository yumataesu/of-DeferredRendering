//modified from https://github.com/jacres/of-DeferredRendering/blob/master/src/pointLight.h

#pragma once
#include "ofMain.h"

class PointLight : public ofNode {
public:
    PointLight() :
    intensity(1.0f)
    {
        sphere.set(10, 10);
        ambient = ofVec3f(0.0, 0.0, 0.0);
        diffuse = ofVec3f(0.0, 0.0, 0.0);
        specular = ofVec3f(0.0, 0.0, 0.0);
        attenuation = ofVec3f(0.0, 0.0);
    }
    
    void setAmbient(float r, float g, float b)
    {
        ambient = ofVec3f(r, g, b);
    }
    
    
    void setDiffuse(float r, float g, float b, float a=1.0f)
    {
        diffuse = ofVec3f(r, g, b);
    }
    
    
    void setSpecular(float r, float g, float b, float a=1.0f)
    {
        specular = ofVec3f(r, g, b);
    }
    
    
    void setAttenuation(float constant, float linear, float exponential)
    {
        attenuation = ofVec3f(constant, linear, exponential);
    }
    
    
    ofVec3f getAmbient() const
    {
        return ambient;
    }
    
    
    ofVec3f getDiffuse() const
    {
        return diffuse;
    }
    
    
    ofVec3f getSpecular() const
    {
        return specular;
    }
    
    
    ofVec3f getAttenuation() const
    {
        return attenuation;
    }
    
    
    void draw()
    {
        sphere.draw();
    }
    
private:
    
    ofVec3f ambient;
    ofVec3f diffuse;
    ofVec3f specular;
    ofVec3f attenuation;
    
    float intensity;
    ofSpherePrimitive sphere;
    
    ofVec3f orbitAxis;
};
