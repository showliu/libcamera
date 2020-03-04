#ifndef GLWINDOW_H
#define GLWINDOW_H

#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <QOpenGLBuffer>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
 
public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    void        loadYUV();
    void        setFrameSize(int width, int height);
    void        updateFrame(unsigned char  *buffer);

protected:
    void        initializeGL() Q_DECL_OVERRIDE; 
    void        paintGL() Q_DECL_OVERRIDE;
    void        resizeGL(int w, int h) Q_DECL_OVERRIDE;

private:
    QOpenGLShader *pVShader;
    QOpenGLShader *pFShader;
    QOpenGLShaderProgram shaderProgram;

    GLuint textureUniformY;
    GLuint textureUniformU;
    GLuint textureUniformV;
    GLuint id_y;
    GLuint id_u;
    GLuint id_v;
    QOpenGLTexture textureY;
    QOpenGLTexture textureU;
    QOpenGLTexture textureV;

    int frameW;
    int frameH;
    unsigned char* yuvDataPtr;
    
    QOpenGLBuffer glBuffer;
};
#endif // GLWINDOW_H
