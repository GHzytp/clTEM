#include <utility>

#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QWidget>
#include <QtWidgets>
#include <QOpenGLWidget>
#include <QtOpenGL>

#include <memory>

#include "GL/glu.h"

#include "arraybuffer.h"
#include "camera.h"
#include "framebuffer.h"


#include "techniques/rectangleshader.h"
#include "techniques/scattershader.h"

#include <techniques/scattertechnique.h>
#include <techniques/rectangletechnique.h>

#include <Eigen/Dense>

namespace View {
    enum Direction {
        Front,
        Back,
        Left,
        Right,
        Top,
        Bottom
    };
}

namespace PGL {
    class PlotWidget : public QOpenGLWidget {
        Q_OBJECT

    signals:
        void resetView();

        void initError(std::string);

    public:
        explicit PlotWidget(QWidget *parent, int msaa=1);

        ~PlotWidget() override;

        std::weak_ptr<PGL::Scatter> scatter(std::vector<Eigen::Vector3f> positions, std::vector<Eigen::Vector3f> colours);

        std::weak_ptr<PGL::Rectangle> rectangle(float t, float l, float b, float r, float z, Eigen::Vector4f &colour, PGL::Plane pl);

        Eigen::Matrix<float, 3, 2> GetSceneLimits();

        std::vector<Eigen::Vector3f> GetBoundingCube();

        void FitView(float extend = 1.0);

        void SetViewDirection(View::Direction view_dir);

        void removeItem(std::weak_ptr<PGL::Technique> technique) {
            std::shared_ptr<PGL::Technique> temp = technique.lock();
            auto position = std::find(_techniques.begin(), _techniques.end(), temp);
            if (position != _techniques.end())
                _techniques.erase(position);
        }

        void clearItems() { _techniques.clear(); }

    protected:
        bool event(QEvent *event) override;

        void initializeGL() override;

        void paintGL() override;

        void resizeGL(int width, int height) override;

        void mousePressEvent(QMouseEvent *event) override;

        void mouseMoveEvent(QMouseEvent *event) override;

        void wheelEvent(QWheelEvent *event) override;

        void keyPressEvent(QKeyEvent *event) override;

    private:
        void addItem(std::shared_ptr<PGL::Technique> technique) {
            auto position = std::find(_techniques.begin(), _techniques.end(), technique);
            if (position == _techniques.end()) // i.e. element does not already exist
                _techniques.push_back(technique);
        }

        void FitOrthoView(float extend = 1.0);

        std::shared_ptr<PGL::RectangleShader> _rect_shader;
        std::shared_ptr<PGL::ScatterShader> _scatter_shader;

        std::vector<std::shared_ptr<PGL::Technique>> _techniques;

        std::shared_ptr<PGL::Framebuffer> _framebuffer;

        Eigen::Vector3f directionEnumToVector(View::Direction d);

        std::shared_ptr<PGL::Camera> _camera;

        // width of the openGL window
        float _width, _height;

        int _msaa;

        Eigen::Vector3f _background;

        QPoint _lastPos;

        void SetCamera(Eigen::Vector3f position, Eigen::Vector3f target, Eigen::Vector3f up, Eigen::Vector3f rot, ViewMode mode);

        void SetCamera(Eigen::Vector3f position, Eigen::Vector3f target, Eigen::Vector3f up, ViewMode mode);

        void contextMenuRequest(QPoint pos);

    private slots:
        void resetPressed() { emit resetView(); }
    };
}

#endif // MYGLWIDGET_H
