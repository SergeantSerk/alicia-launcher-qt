#include "Window.hpp"
#include "Launcher.hpp"

#include <QDesktopServices>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMovie>
#include <QPainter>
#include <QtConcurrent>
#include <QWidget>
#include <QFontDatabase>
#include <QStyle>

#include <cmath>

constexpr char const* const WEB_STORYOFALICIA_TICKET = "https://storyofalicia.com/ticket";

namespace ui
{

int start(int argc, char* argv[])
{
  QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
  QApplication application(argc, argv);

  QFontDatabase::addApplicationFont(":/font/not_eurostile.otf");
  QFontDatabase::addApplicationFont(":/font/eurostile.otf");
  QFontDatabase::addApplicationFont(":/font/eurostile-black.otf");
  QFontDatabase::addApplicationFont(":/font/nanum_gothic-extra_bold.ttf");
  QFontDatabase::addApplicationFont(":/font/nanum_gothic-regular.ttf");

  Window window{};
  window.show();

  return QApplication::exec();
}

Window::Window(QWidget* parent)
    : QWidget(parent)
{
  _progressDialog->hide();

  _gameStartMovie = new QMovie(":/img/game_start_hover.gif");
  _gameStartMovie->start();
  _gameStartMovie->setPaused(true);

  _masterFrameUI.setupUi(_masterFrame);
  _masterFrameUI.l_game_start->setMovie(_gameStartMovie);

  // login widget
  _loginWidgetUI.setupUi(_masterFrameUI.login_widget);

  _loginWidgetUI.l_error->hide();
  _loginWidgetUI.l_error_user->hide();
  _loginWidgetUI.l_error_user->setAttribute(Qt::WA_TransparentForMouseEvents);
  _loginWidgetUI.l_error_pass->hide();
  _loginWidgetUI.l_error_pass->setAttribute(Qt::WA_TransparentForMouseEvents);

  _masterFrameUI.login_widget->show();

  // menu widget
  _menuWidgetUI.setupUi(_masterFrameUI.menu_widget);
  _masterFrameUI.menu_widget->hide();

  // transparent frames
  _masterFrameUI.l_frame->setAttribute(Qt::WA_TransparentForMouseEvents);
  _masterFrameUI.l_game_start_frame->setAttribute(Qt::WA_TransparentForMouseEvents);

  // main window attributes
  setWindowFlags( Qt::Window | Qt::FramelessWindowHint);

  setAttribute(Qt::WA_TranslucentBackground, true);
  setAttribute(Qt::WA_NoSystemBackground, true);

  _masterFrame->setAttribute(Qt::WA_TranslucentBackground, true);
  _masterFrame->setAttribute(Qt::WA_NoSystemBackground, true);

  _masterFrameUI.content->setAttribute(Qt::WA_TranslucentBackground, true);
  _masterFrameUI.content->setAttribute(Qt::WA_NoSystemBackground, true);

  // event filters
  _masterFrameUI.l_game_start->setMouseTracking(true);
  _masterFrameUI.l_game_start->installEventFilter(this);
  _masterFrameUI.l_game_start_frame->installEventFilter(this);

  connect(_masterFrameUI.btn_exit, SIGNAL(clicked()), this, SLOT(handle_exit()));
  connect(_masterFrameUI.btn_minimize, SIGNAL(clicked()), this, SLOT(handle_minimize()));
  connect(_masterFrameUI.btn_settings, SIGNAL(clicked()), this, SLOT(handle_settings()));

  connect(_masterFrameUI.btn_repair, SIGNAL(clicked()), this, SLOT(handle_repair()));
  connect(_masterFrameUI.btn_ticket, SIGNAL(clicked()), this, SLOT(handle_ticket()));

  connect(_loginWidgetUI.btn_login, SIGNAL(clicked()), this, SLOT(handle_login()));
  connect(_menuWidgetUI.btn_logout, SIGNAL(clicked()), this, SLOT(handle_logout()));

  connect(_menuWidgetUI.btn_info, SIGNAL(clicked()), this, SLOT(handle_info()));

  connect(_progressDialog->_ui_progressWidget.btn_pause, SIGNAL(clicked()), this, SLOT(handle_install_pause()));
  connect(_progressDialog->_ui_progressWidget.btn_close, SIGNAL(clicked()), this, SLOT(handle_install_stop()));

  connect(_menuWidgetUI.btn_info, SIGNAL(clicked()), this, SLOT(handle_info()));

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
  // handling paint events for _masterFrameUI.l_game_start_frame
  if (object == _masterFrameUI.l_game_start_frame && event->type() == QEvent::Paint)
  {
    auto *label = dynamic_cast<QLabel*>(object);
    QPainter painter(label);

    const auto pixmap = label->pixmap().scaled(label->size());
    label->style()->drawItemPixmap(&painter, label->rect(), Qt::AlignCenter, pixmap);
    return true;
  }

  // handling MouseButtonPress and MouseMove for _masterFrameUI.l_game_start
  if (
    object == _masterFrameUI.l_game_start && (event->type() == QEvent::MouseMove) ||
    (event->type() == QEvent::MouseButtonPress))
  {
    if (!dynamic_cast<QWidget*>(object)->isEnabled())
      return true;

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
        _gameStartMovie->setPaused(false);
      }
      else if (
        event->type() == QEvent::MouseButtonPress &&
        dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
      {
        // stopping the animation so it doesn't get stuck replaying
        _shouldAnimateGameStart = false;
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
  // todo: implement
}

void Window::handle_ticket() { QDesktopServices::openUrl(QString(WEB_STORYOFALICIA_TICKET)); }

void Window::handle_logout()
{
  _launcher.logout();
  _masterFrameUI.login_widget->show();
  _masterFrameUI.menu_widget->hide();
}

// maybe remove slot specifier
void Window::handle_launch()
{
  if (_workerRunning)
    return;

  if (!_launcher.isAuthenticated())
    return;

  _workerRunning = true;
  _workerThread = std::make_unique<std::thread>(
    [this]
    {
      bool isUpdated = false;

      if (!_launcher.checkFiles())
      {
        bool shouldUpdate = false;
        int toDownload = _launcher.countToDownload();
        QMetaObject::invokeMethod(
          this,
          [this, toDownload]()
          {
            QMessageBox box(this);
            box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            box.setIcon(QMessageBox::Icon::Critical);
            box.setText(QString("%1 files need patching. Patch them?").arg(toDownload));
            return box.exec() == QMessageBox::Yes;
          },
          Qt::BlockingQueuedConnection,
          qReturnArg(shouldUpdate));

        // shouldUpdate will be set to true, if user picked the yes option
        if (shouldUpdate)
        {
          QMetaObject::invokeMethod(this, [this]
          {
            this->_progressDialog->begin(_masterFrameUI.content, "Updating");
            this->_masterFrameUI.l_game_start_frame->setDisabled(false);
          }, Qt::BlockingQueuedConnection);

          int to_download = _launcher.countToDownload();
          int to_patch = to_download;
          int all_files = _launcher.countToDownload() * 2;

          int downloaded = 0;
          int patched = 0;
          int overall_done = 0;

          try
          {
            while(_launcher.countToDownload() > 0)
            {
              downloaded++;
              if(_launcher.isUpdateStopped())
              {
                break;
              }

              _launcher.downloadNextFile([this, downloaded, to_download](int progress, std::string name) -> void
              {
                QMetaObject::invokeMethod(this, [this, progress, name = std::move(name), downloaded, to_download]() mutable
                {
                  std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                  this->_progressDialog->updateSecondary(progress, QString("DOWNLOADING '%1' (%2/%3)").arg(name.data()).arg(downloaded).arg(to_download));
                }, Qt::QueuedConnection);
              });

              overall_done++;
              QMetaObject::invokeMethod(this, [this, all_files, overall_done]()
              {
                this->_progressDialog->updatePrimary(static_cast<int>((static_cast<double>(overall_done) / static_cast<double>(all_files)) * 100));
              }, Qt::QueuedConnection);
            }

            while(_launcher.countToPatch() > 0)
            {
              patched++;
              if(_launcher.isUpdateStopped())
              {
                break;
              }

              _launcher.patchNextFile([this, to_patch, patched](int progress, std::string name) -> void
              {
                QMetaObject::invokeMethod(this, [this, progress, name = std::move(name), to_patch, patched]() mutable
                {
                  std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                  this->_progressDialog->updateSecondary(progress, QString("PATCHING '%1' (%2/%3)").arg(name.data()).arg(patched).arg(to_patch));
                }, Qt::QueuedConnection);
              });
              overall_done++;
              QMetaObject::invokeMethod(this, [this, all_files, overall_done]()
              {
                this->_progressDialog->updatePrimary(static_cast<int>((static_cast<double>(overall_done) / static_cast<double>(all_files)) * 100));
              }, Qt::QueuedConnection);
            }

            if (_launcher.isUpdateStopped())
            {
              QMetaObject::invokeMethod(this, [this]
              {
                this->_progressDialog->end();
              }, Qt::QueuedConnection);
              isUpdated = false; // not finished updating
            } else
            {
              isUpdated = true; // updated
              auto thread = std::thread([this]
              {
                QMetaObject::invokeMethod(this, [this]()
                {
                  this->_progressDialog->updateSecondary(100, QString("FINISHED"));
                  this->_progressDialog->updatePrimary(100);
                }, Qt::BlockingQueuedConnection);
                // make it pretty
                std::this_thread::sleep_for(std::chrono::milliseconds(600));

                QMetaObject::invokeMethod(this, [this]
                {
                  this->_progressDialog->end();
                }, Qt::QueuedConnection);
              });
              thread.detach();
            }
          }
          catch (const std::exception& e)
          {
            // TODO: log patching error
            isUpdated = false; // interrupted by error
          }
        }
      } else
      {
        isUpdated = true; // all files have the correct checksum
      }

      // if the files were recently patched
      if (isUpdated)
      {
        //QMetaObject::invokeMethod(this, [this] { this->hide(); }, Qt::QueuedConnection);
        //if (launcher::launch(this->_profile))
        //{
          //std::this_thread::sleep_for(std::chrono::seconds(5));
        //}
        //else
        //{
          // TODO: log error
        //}

        //QMetaObject::invokeMethod(this, [this] { handle_exit(); }, Qt::QueuedConnection);
      }

      this->_workerRunning = false;
    });
  _workerThread->detach();
}

void Window::handle_post_login()
{
  if (_launcher.isAuthenticated())
  {
    _menuWidgetUI.l_username_d->setText(QString(_launcher.profile().username.data()));
    _menuWidgetUI.l_player_d->setText(QString(_launcher.profile().character_name.data()));

    _menuWidgetUI.l_level_d->setText(QString::number(_launcher.profile().level));
    _menuWidgetUI.l_guild_d->setText(QString(_launcher.profile().guild.data()));
    auto const date = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(_launcher.profile().last_login));
    _menuWidgetUI.l_last_login_d->setText(date.toString("yy-MM-dd HH:mm:ss"));

    _masterFrameUI.menu_widget->show();
    _masterFrameUI.login_widget->hide();
  }
  else
  {
    //_loginWidgetUI.l_error->setText("Authentication failed");
    _loginWidgetUI.l_error->show();
    _loginWidgetUI.l_error_pass->show();
    _loginWidgetUI.l_error_user->show();
  }
  _masterFrameUI.login_widget->setDisabled(false);
}

void Window::handle_login()
{
  if (_workerRunning)
    return;

  _loginWidgetUI.l_error->hide();
  _loginWidgetUI.l_error_pass->hide();
  _loginWidgetUI.l_error_user->hide();

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
        _launcher.authenticate(username, password);
        QMetaObject::invokeMethod(this, Window::handle_post_login, Qt::QueuedConnection);
      }
      catch (std::exception& e)
      {
        QMetaObject::invokeMethod(this, Window::handle_post_login, Qt::QueuedConnection);
      }

      // release mutex
      this->_workerRunning = false;
    });

  _workerThread->detach();
}

void Window::handle_info()
{
  // TODO: implement info
}

void Window::handle_install_pause()
{
  _launcher.setUpdatePaused(!_launcher.isUpdatePaused());
}

void Window::handle_install_stop()
{
  _launcher.setUpdatePaused(false); // resume, to be able to stop
  _launcher.stopUpdate();
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
