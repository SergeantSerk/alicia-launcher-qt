#include "Window.hpp"
#include "Launcher.hpp"

#include <QDialog>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QMovie>
#include <QPainter>
#include <QtConcurrent>
#include <QWidget>

#include <cmath>

namespace ui
{

int start(int argc, char* argv[])
{
  QApplication application(argc, argv);

  Window window{};
  window.show();

  return QApplication::exec();
}

Window::Window(QWidget* parent)
    : QWidget(parent)
{
  _gameStartMovie = new QMovie(":/img/game_start_hover.gif");

  // to display frame 0, so the button is not empty
  _gameStartMovie->start();
  _gameStartMovie->setPaused(true);

  _masterFrameUI.setupUi(_masterFrame);

  _masterFrameUI.l_game_start->setMovie(_gameStartMovie);

  _loginWidgetUI.setupUi(_masterFrameUI.login_widget);
  _loginWidgetUI.l_error->hide();

  _masterFrameUI.login_widget->show();

  _menuWidgetUI.setupUi(_masterFrameUI.menu_widget);
  _masterFrameUI.menu_widget->hide();

  connect(_masterFrameUI.btn_exit, SIGNAL(clicked()), this, SLOT(handle_exit()));
  connect(_masterFrameUI.btn_minimize, SIGNAL(clicked()), this, SLOT(handle_minimize()));
  connect(_masterFrameUI.btn_settings, SIGNAL(clicked()), this, SLOT(handle_settings()));

  connect(_masterFrameUI.btn_repair, SIGNAL(clicked()), this, SLOT(handle_repair()));
  connect(_masterFrameUI.btn_ticket, SIGNAL(clicked()), this, SLOT(handle_ticket()));

  connect(_loginWidgetUI.btn_login, SIGNAL(clicked()), this, SLOT(handle_login()));
  connect(_menuWidgetUI.btn_logout, SIGNAL(clicked()), this, SLOT(handle_logout()));

  connect(_menuWidgetUI.btn_info, SIGNAL(clicked()), this, SLOT(handle_info()));

  setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
  setAttribute(Qt::WA_NoSystemBackground, true);
  setAttribute(Qt::WA_TranslucentBackground, true);

  // handle l_game_start_frame mouse tracking, because it sits at the top of Z-axis
  _masterFrameUI.l_game_start_frame->setMouseTracking(true);
  _masterFrameUI.l_game_start_frame->installEventFilter(this);

  // to be able to stop the animation cleanly at frame 0
  connect(_gameStartMovie, SIGNAL(frameChanged(int)), this, SLOT(handle_frame_changed(int)));
}

void Window::mousePressEvent(QMouseEvent* event)
{
  _windowDragActive = true;
  _mouseEventPos = event->pos();
}

void Window::mouseReleaseEvent(QMouseEvent* event) { _windowDragActive = false; }

void Window::mouseMoveEvent(QMouseEvent* event)
{
  if (!_windowDragActive)
    return;

  move(event->globalPosition().toPoint() - _mouseEventPos);
}

bool Window::eventFilter(QObject* object, QEvent* event)
{
  // handling MouseButtonPress and MouseMove for _masterFrameUI.l_game_start_frame
  if (
    object == _masterFrameUI.l_game_start_frame && (event->type() == QEvent::MouseMove) ||
    (event->type() == QEvent::MouseButtonPress))
  {
    // sqrt((x1 - x2)^2 + (y1 - y2)^2)
    // using "scene" coordinates (window coordinates)
    double const distance = std::sqrt(
      std::pow(
        dynamic_cast<QMouseEvent*>(event)->scenePosition().x() -
          _masterFrameUI.l_game_start->geometry().center().x(),
        2) +
      std::pow(
        dynamic_cast<QMouseEvent*>(event)->scenePosition().y() -
          _masterFrameUI.l_game_start->geometry().center().y(),
        2));

    // game start button radius is 119 pixels
    if (distance <= 119)
    {
      if (event->type() == QEvent::MouseMove)
      {
        // playing animation when the mouse is within the button
        _shouldAnimateGameStart = true;
        _gameStartMovie->start();
      }
      else if (event->type() == QEvent::MouseButtonPress)
      {
        // launching the game when the mouse is pressed within the button
        handle_launch();
      }
    }
    else
    {
      if (event->type() == QEvent::MouseMove)
      {
        // signal to stop the animation when the mouse leaves the button
        _shouldAnimateGameStart = false;
      }
    }
  }

  return false;
}

void Window::handle_exit() { QCoreApplication::quit(); }

void Window::handle_minimize() { showMinimized(); }

void Window::handle_settings()
{
  // TODO: implement settings
}

void Window::handle_repair()
{
  // TODO: implement repair
}

void Window::handle_ticket()
{
  // TODO: implement ticket
}

void Window::handle_logout()
{
  this->_authorized = false;
  _masterFrameUI.login_widget->show();
  _masterFrameUI.menu_widget->hide();
}

// maybe remove slot specifier
void Window::handle_launch()
{
  if (_workerRunning)
    return;

  if (!_authorized)
    return;

  _workerRunning = true;
  _workerThread = std::make_unique<std::thread>(
    [this]
    {
      if (const auto files = launcher::fileCheck(); !files.empty())
      {
        QMetaObject::invokeMethod(
          this,
          [this]()
          {
            QDialog d(this);
            d.exec();
            // TODO: ui
          },
          Qt::QueuedConnection);
      }
      else if (!launcher::launch(this->profile.load()))
      {
        //TODO: launch
      }

      this->_workerRunning = false;
    });
  _workerThread->detach();
}

void Window::handle_login()
{
  if (_workerRunning)
    return;

  _masterFrameUI.login_widget->setDisabled(true);

  auto const username = _loginWidgetUI.input_username->text().toStdString();
  auto const password = _loginWidgetUI.input_password->text().toStdString();

  // lock mutex
  _workerRunning = true;
  // authorization leaves the Qt Application thread, calls Qt functions trough
  // QMetaObject::invokeMethod
  _workerThread = std::make_unique<std::thread>(
    [username, password, this]() -> void
    {
      try
      {
        this->profile = launcher::authenticate(username, password);

        QMetaObject::invokeMethod(
          this,
          [this]
          {
            this->_masterFrameUI.menu_widget->show();
            this->_masterFrameUI.login_widget->hide();
            this->_authorized = true;
          },
          Qt::QueuedConnection);
      }
      catch (std::exception& e)
      {
        QMetaObject::invokeMethod(
          this, [this] { this->_loginWidgetUI.l_error->show(); }, Qt::QueuedConnection);
      }

      QMetaObject::invokeMethod(
        this, [this] { _masterFrameUI.login_widget->setDisabled(false); }, Qt::QueuedConnection);
      // release mutex
      this->_workerRunning = false;
    });

  _workerThread->detach();
}

void Window::handle_info()
{
  // TODO: implement info
}

void Window::handle_frame_changed(int frameNumber)
{
  if (!_shouldAnimateGameStart && frameNumber == 0)
  {
    // stop the playback at frame 0, when _shouldAnimateGameStart is false
    _gameStartMovie->setPaused(true);
  }
}

} // namespace ui
