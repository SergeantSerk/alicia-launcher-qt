#include "MasterWindow.hpp"

#include <QMessageBox>
#include <QWidget>
#include <QFontDatabase>
#include <format>

#include "launcher.hpp"
#include "util.hpp"
#include <QFile>

namespace Ui
{

MasterWindow::MasterWindow(QWidget* parent)
    : QWidget(parent)
{
  _widget.setupUi(this);
  connect(_widget.btn_exit, SIGNAL(clicked()), this, SLOT(handle_btn_exit()));
  connect(_widget.btn_login, SIGNAL(clicked()), this, SLOT(handle_login()));
  setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
  setAttribute(Qt::WA_NoSystemBackground, true);
  setAttribute(Qt::WA_TranslucentBackground, true);

  _widget.frame->setFrameStyle(QFrame::StyledPanel);
  _widget.login_frame->show();
}

void MasterWindow::mousePressEvent(QMouseEvent *event)
{
    _windowDragActive = true;
    _mouseEventPos = event->pos();
}

void MasterWindow::mouseReleaseEvent(QMouseEvent *event)
{
  _windowDragActive = false;
}

void MasterWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!_windowDragActive)
      return;

    move(event->globalPosition().toPoint() - _mouseEventPos);
}

void MasterWindow::handle_btn_exit()
{
  QCoreApplication::quit();
}

void MasterWindow::handle_login()
{
  this->_widget.login_frame->hide();
  this->_widget.
}

void MasterWindow::handle_btn_launch()
{
  const std::string _webInfoId = "927628CA6D76A6E9162C56D4E3E6D6E3";

  const launcher::WebInfo _webInfoContent{
    .gameId = "Alicia",
    .memberNo = 0,
    .loginId = "<name>",
    .authKey = "<auth_key>",
    .installUrl = "<install_url>",
    .serverType = 0,
    .serverInfo = ":",
    .sex = launcher::WebInfo::Sex::MALE,
    .birthday = "",
    .wardNo = 0,
    .cityCode = 0,
    .zipCode = "",
    .pcBangNo = 0,
    .closeTime = "",
  };

  std::string _executableProgram = "Alicia.exe";
  std::string _workdir = R"(C:\Users\martin\AppData\Local\Gametree\Alicia)";
  std::string _executableArguments = "-GameID 0";

  try
  {
    launcher::WebInfoHost webInfoHost;
    webInfoHost.host(_webInfoId, _webInfoContent);
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Error", "Couldn't host the web info.");
    return;
  }

  STARTUPINFO startupInfo{.cb = sizeof(STARTUPINFO)};
  PROCESS_INFORMATION processInfo{};

  SetCurrentDirectory(util::win32_widen(_workdir).data());
  if (!CreateProcess(
        util::win32_widen(_executableProgram).data(),
        util::win32_widen(_executableArguments).data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo))
  {
    if (GetLastError() == ERROR_ELEVATION_REQUIRED)
    {
      QMessageBox::critical(this, "Error", "Can't launch the game, elevation is required.");
    }
    else
    {
      QMessageBox::critical(this, "Error", "Can't launch the game, misc error.");
    }
  }
  else
  {
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    QMessageBox::information(this, "Info", std::format("Game exited with code {}.", exitCode).data());

    CloseHandle(&processInfo);
    exit(0);
  }
}
} // namespace Ui
