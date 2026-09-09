#include "configuration.h"
#include "inputMappingDialog.h"
#include "patchesDialog.h"
#include "trophyViewerDialog.h"
#include "compatibilityDatabase.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTimer>
#include <QtEndian>
#include "launcherTheme.h"
#include "libraryController.h"
#include "librarySettings.h"
#include "mainDialog.h"
#include "uiTranslations.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJSValue>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWidget>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class LibraryTests : public QObject {
  Q_OBJECT
  QTemporaryDir m_directory;
  QString m_old_directory;
  QString m_game;
  QString Fixture() const {
#ifdef _WIN32
    return QCoreApplication::applicationDirPath() +
           "/library_process_fixture.exe";
#else
    return QCoreApplication::applicationDirPath() + "/library_process_fixture";
#endif
  }
  static QString Logs(LibraryController &controller) {
    QStringList lines;
    auto *model = controller.LogModel();
    for (int i = 0; i < model->rowCount(); ++i)
      lines.append(model->data(model->index(i, 0), Qt::DisplayRole).toString());
    return lines.join('\n');
  }
private slots:
  void initTestCase() {
    QVERIFY(m_directory.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.filePath("user"));
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_directory.filePath("system"));
    m_old_directory = QDir::currentPath();
    QVERIFY(QDir::setCurrent(m_directory.path()));
    m_game = m_directory.filePath("Games & spaces/Test title");
    QVERIFY(QDir().mkpath(m_game + "/sce_sys"));
    QFile eboot(m_game + "/eboot.bin");
    QVERIFY(eboot.open(QIODevice::WriteOnly));
    eboot.write("fixture");
    QFile param(m_game + "/sce_sys/param.json");
    QVERIFY(param.open(QIODevice::WriteOnly));
    param.write(
        R"({"titleId":"PPSA00001","appVersion":"01.000","localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"Library test game"}}})");
  }
  void init() {
    QSettings settings("Kyty.ini", QSettings::IniFormat);
    settings.clear();
    settings.setValue("Launcher/game_dirs",
                      QStringList{m_directory.filePath("Games & spaces")});
    settings.setValue("MainDialog/check_updates_on_startup", false);
    settings.sync();
  }
  void settingsValidationAndRoundTrip() {
    Configuration source;
    const auto fields = LibrarySettings::Fields(source);
    QVariantMap values;
    for (const auto &value : fields)
      values.insert(value.toMap()["key"].toString(), value.toMap()["value"]);
    values["readback_linear_images"] = true;
    values["console_language"] = 5;
    values["screen_resolution"] = "1920x1080";
    values["user_name"] = "Fran";
    values["printf_direction"] = "File";
    values["printf_output_file"] = "logs/test output.txt";
    QVERIFY(LibrarySettings::Apply(source, values).isEmpty());
    QVERIFY(source.readback_linear_images);
    const auto args = CreateEmulatorArgs(source);
    const auto readback = args.indexOf("--readback-linear-images");
    QVERIFY(readback >= 0);
    QCOMPARE(args.value(readback + 1), QString("true"));
    QCOMPARE(source.console_language, 5);
    QCOMPARE(source.screen_resolution, Configuration::Resolution::R1920X1080);
    QSettings saved("roundtrip.ini", QSettings::IniFormat);
    source.WriteSettings(&saved);
    Configuration restored;
    restored.ReadSettings(&saved);
    QCOMPARE(restored.printf_output_file, source.printf_output_file);
    QCOMPARE(LibrarySettings::Fields(restored),
             LibrarySettings::Fields(source));
    values["user_id"] = 255;
    QVERIFY(!LibrarySettings::Apply(restored, values).isEmpty());
    values["user_id"] = 1000;
    values["screen_resolution"] = "invalid";
    QVERIFY(!LibrarySettings::Apply(restored, values).isEmpty());
  }
  void gpuSelectionArgumentsAndPersistence() {
    Configuration config;
    QVERIFY(!CreateEmulatorArgs(config).contains("--gpu"));
    config.gpu_index = 1;
    auto args = CreateEmulatorArgs(config);
    const int option = args.indexOf("--gpu");
    QVERIFY(option >= 0);
    QCOMPARE(args.value(option + 1), QString("1"));
    Configuration copied;
    copied.CopyEmulatorSettingsFrom(config);
    QCOMPARE(copied.gpu_index, 1);
    QSettings settings("gpu-roundtrip.ini", QSettings::IniFormat);
    config.WriteSettings(&settings);
    Configuration restored;
    restored.ReadSettings(&settings);
    QCOMPARE(restored.gpu_index, 1);
    QVERIFY(LibrarySettings::Apply(restored, {{"gpu_selection", 0}}).isEmpty());
    QCOMPARE(restored.gpu_index, -1);
    QVERIFY(!LibrarySettings::Apply(restored, {{"gpu_selection", -1}}).isEmpty());
  }
  void configurationInheritance() {
    LibraryController controller(nullptr);
    QCOMPARE(controller.Games().size(), 1);
    controller.selectGame(m_game);
    QVERIFY(controller.saveSettings(true, {{"console_language", 5}}).isEmpty());
    QVERIFY(
        controller.saveSettings(false, {{"console_language", 1}}).isEmpty());
    QVERIFY(controller.saveSettings(true, {{"console_language", 9}}).isEmpty());
    const auto values = controller.settings(false);
    for (const auto &field : values) {
      if (field.toMap()["key"] == "console_language")
        QCOMPARE(field.toMap()["value"].toInt(), 1);
    }
    QVERIFY(controller.Selected()["custom"].toBool());
  }
  void processLogsAndRestart() {
    LibraryController controller(nullptr);
    controller.Initialize(Fixture());
    QTRY_VERIFY_WITH_TIMEOUT(controller.Ready(), 5000);
    controller.start();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.Running(), 5000);
    const auto text = Logs(controller);
    QVERIFY2(text.contains(QString::fromUtf8("stdout €")), qPrintable(text));
    QVERIFY(text.contains("stderr captured"));
    QVERIFY(text.contains("final partial line"));
    QVERIFY(!text.contains(QChar(0x1b)));
    QVERIFY(text.contains(m_game));
    QCOMPARE(controller.Status(), QString("Session finished"));
    controller.clearLog();
    QVERIFY(controller.saveSettings(true, {{"user_name", "fail"}}).isEmpty());
    controller.start();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.Running(), 5000);
    QVERIFY(Logs(controller).contains("exit code 7"));
    QCOMPARE(controller.Status(), QString("Emulator exited with an error"));
    controller.filterLog("stderr");
    QCOMPARE(controller.LogModel()->rowCount(), 1);
  }
  void stopAndGuardSettings() {
    LibraryController controller(nullptr);
    controller.Initialize(Fixture());
    QTRY_VERIFY_WITH_TIMEOUT(controller.Ready(), 5000);
    QVERIFY(controller.saveSettings(true, {{"user_name", "slow"}}).isEmpty());
    controller.start();
    QTRY_COMPARE_WITH_TIMEOUT(controller.Status(), QString("Running"), 5000);
    QVERIFY(
        !controller.saveSettings(true, {{"user_name", "changed"}}).isEmpty());
    controller.start(); // A second Start must not spawn another process.
    controller.stop();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.Running(), 6000);
    QCOMPARE(controller.Status(), QString("Stopped"));
  }
  void launchFailureAndMissingExecutable() {
    LibraryController missing(nullptr);
    missing.Initialize(m_directory.filePath("missing-emulator"));
    QVERIFY(!missing.Ready());
    QVERIFY(Logs(missing).contains("Place"));
    const auto temporary = m_directory.filePath(
#ifdef _WIN32
        "temporary-emulator.exe"
#else
        "temporary-emulator"
#endif
    );
    QVERIFY(QFile::copy(Fixture(), temporary));
    LibraryController controller(nullptr);
    controller.Initialize(temporary);
    QTRY_VERIFY_WITH_TIMEOUT(controller.Ready(), 5000);
    QVERIFY(QFile::remove(temporary));
    controller.start();
    QTRY_COMPARE_WITH_TIMEOUT(controller.Status(),
                              QString("Unable to start emulator"), 5000);
    QVERIFY(!controller.Running());
  }
  void boundedConsole() {
    LibraryLogModel model;
    QStringList rows;
    for (int i = 0; i < 7000; ++i)
      rows.append(QString::number(i));
    model.Append(rows);
    QCOMPARE(model.rowCount(), 6000);
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(),
             QString("1000"));
    model.Append({"last"});
    QCOMPARE(model.rowCount(), 6000);
    model.Clear();
    QCOMPARE(model.rowCount(), 0);
  }
  void qmlScreens() {
    const auto qa_games = qEnvironmentVariable("KYTY_LIBRARY_QA_GAMES");
    if (!qa_games.isEmpty()) {
      QSettings settings("Kyty.ini", QSettings::IniFormat);
      settings.setValue("Launcher/game_dirs", QStringList{qa_games});
      settings.sync();
    }
    MainDialog window;
    auto *quick = window.findChild<QQuickWidget *>();
    QVERIFY(quick);
    for (const auto &error : quick->errors())
      qWarning("%s", qPrintable(error.toString()));
    QCOMPARE(quick->status(), QQuickWidget::Ready);
    window.resize(1280, 820);
    window.show();
    QTest::qWait(400);
    auto *root = quick->rootObject();
    QVERIFY(root);
    const auto output = QCoreApplication::applicationDirPath() + "/library-qa";
    QDir().mkpath(output);
    QVERIFY(quick->grabFramebuffer().save(output + "/library.png"));
    QVERIFY(QMetaObject::invokeMethod(root, "showOptions",
                                      Q_ARG(QVariant, QVariant(true))));
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/options.png"));
    // Exercise the dynamically loaded controls, not only the options landing
    // page.
    auto *settings = root->findChild<QObject *>("settingsPage");
    QVERIFY(settings);
    settings->setProperty("category", "Graphics");
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/graphics.png"));
    settings->setProperty("category", "System");
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/system.png"));
    const auto find_visual = [](auto &&self, QQuickItem *item,
                                const QString &name) -> QQuickItem * {
      if (item->objectName() == name)
        return item;
      for (auto *child : item->childItems()) {
        if (auto *found = self(self, child, name))
          return found;
      }
      return nullptr;
    };
    settings->setProperty("category", "Graphics");
    QTest::qWait(50);
    auto *resolution = find_visual(find_visual, root, "editor_screen_resolution");
    QVERIFY(resolution);
    auto *popup = resolution->property("popup").value<QObject *>();
    QVERIFY(popup);
    QVERIFY(QMetaObject::invokeMethod(popup, "open"));
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/dropdown.png"));
    QVERIFY(QMetaObject::invokeMethod(popup, "close"));
    settings->setProperty("category", "System");
    QTest::qWait(50);
    auto *name_editor = find_visual(find_visual, root, "editor_user_name");
    QVERIFY(name_editor);
    name_editor->setProperty("text", "UI test");
    QVERIFY(QMetaObject::invokeMethod(name_editor, "textEdited"));
    QCOMPARE(settings->property("draft")
                 .value<QJSValue>()
                 .property("user_name")
                 .toString(),
             QString("UI test"));
    QVERIFY(settings->property("dirty").toBool());
    settings->setProperty("dirty", false);
    root->setProperty("options", false);
    root->setProperty("consoleOpen", true);
    window.resize(980, 640);
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/compact-console.png"));
    root->setProperty("listMode", true);
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/list.png"));
    auto *start = find_visual(find_visual, root, "startButton");
    auto *console = find_visual(find_visual, root, "consolePanel");
    QVERIFY(start && console);
    for (int height : {640, 700, 820, 900}) {
      window.resize(980, height);
      QTest::qWait(50);
      const auto buttonBottom = start->mapToScene(QPointF(0, start->height())).y();
      const auto consoleTop = console->mapToScene(QPointF(0, 0)).y();
      QVERIFY(buttonBottom <= consoleTop);
      QVERIFY(start->height() >= 40);
    }
    auto *railItem = find_visual(find_visual, root, "gameRail");
    auto *titleItem = find_visual(find_visual, root, "gameTitle");
    QVERIFY(railItem && titleItem);
    for (int height : {640, 820, 900}) {
      window.resize(980, height);
      root->setProperty("consoleOpen", false);
      QTest::qWait(50);
      const auto titlePosition = titleItem->mapToScene(QPointF());
      const auto railPosition = railItem->mapToScene(QPointF());
      const auto titleHeight = titleItem->height();
      root->setProperty("consoleOpen", true);
      root->setProperty("consoleHeight", 2000);
      QTest::qWait(50);
      QCOMPARE(titleItem->mapToScene(QPointF()), titlePosition);
      QCOMPARE(railItem->mapToScene(QPointF()), railPosition);
      QCOMPARE(titleItem->height(), titleHeight);
      QVERIFY(start->mapToScene(QPointF(0, start->height())).y() < console->mapToScene(QPointF()).y());
    }
    root->setProperty("consoleHeight", 160);
    auto *handle = find_visual(find_visual, root, "consoleResizeHandle");
    QVERIFY(handle);
    window.resize(1280, 900);
    QTest::qWait(50);
    const qreal oldHeight = console->height();
    const QPoint grip = handle->mapToScene(QPointF(handle->width() / 2, 4)).toPoint();
    QTest::mousePress(quick, Qt::LeftButton, Qt::NoModifier, grip);
    QTest::mouseMove(quick, grip - QPoint(0, 60), 50);
    QTest::mouseRelease(quick, Qt::LeftButton, Qt::NoModifier, grip - QPoint(0, 60));
    QTest::qWait(50);
    QVERIFY(console->height() > oldHeight + 30);
    QVERIFY(start->mapToScene(QPointF(0, start->height())).y() < console->mapToScene(QPointF()).y());
    window.close();
  }
  void integratedWindowControls() {
    MainDialog window;
    auto *quick = window.findChild<QQuickWidget *>();
    QVERIFY(quick);
    QCOMPARE(quick->status(), QQuickWidget::Ready);
    QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
    window.resize(1280, 820);
    window.show();
    QTest::qWait(100);
#ifdef Q_OS_WIN
    QVERIFY(window.mask().isEmpty());
#else
    QVERIFY(!window.mask().isEmpty());
    QVERIFY(!window.mask().contains(QPoint(0, 0)));
    QVERIFY(window.mask().contains(window.rect().center()));
#endif
    auto *maximize =
        quick->rootObject()->findChild<QObject *>("maximizeButton");
    QVERIFY(maximize);
    for (const char *name : {"minimizeButton", "maximizeButton", "closeButton"}) {
      auto *button = quick->rootObject()->findChild<QQuickItem *>(name);
      QVERIFY(button);
      const auto point = button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint();
      QTest::mouseMove(quick, point, 20);
      QTest::qWait(30);
      auto *background = button->property("background").value<QObject *>();
      QVERIFY(background);
      QVERIFY(background->property("color").value<QColor>().alpha() > 0);
    }
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                      QPoint(quick->width() - 69, 22));
    QTRY_VERIFY(window.isMaximized());
    QVERIFY(maximize->property("restored").toBool());
    QVERIFY(window.mask().isEmpty());
    QVERIFY(!window.beginWindowResize(Qt::LeftEdge));
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                      QPoint(quick->width() - 69, 22));
    QTRY_VERIFY(!window.isMaximized());
    QVERIFY(!maximize->property("restored").toBool());
    QVERIFY(!window.mask().contains(QPoint(0, 0)));
    window.resize(1080, 720);
    QTest::qWait(20);
    QVERIFY(!window.mask().contains(window.rect().bottomRight()));
#ifndef Q_OS_WIN
    QVERIFY(window.mask().contains(window.rect().center()));
#endif
    QTest::mouseDClick(quick, Qt::LeftButton, Qt::NoModifier, QPoint(350, 22));
    QTRY_VERIFY(window.isMaximized());
    window.toggleMaximized();
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                      QPoint(quick->width() - 115, 22));
    QTRY_VERIFY(window.isMinimized());
    window.showNormal();
    QVERIFY(!window.beginWindowResize(0));
    QVERIFY(!window.beginWindowResize(Qt::LeftEdge | Qt::RightEdge));
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                      QPoint(quick->width() - 23, 22));
    QTRY_VERIFY(!window.isVisible());
  }
  void interfaceLanguages() {
    UiTranslations::Instance().SetLanguage("en");
    MainDialog window;
    window.show();
    QTest::qWait(100);
    auto *controller = window.findChild<LibraryController *>();
    auto *quick = window.findChild<QQuickWidget *>();
    QVERIFY(controller && quick);
    QCOMPARE(controller->UiLanguage(), QString("en"));
    QVERIFY(controller->UiLanguages().size() >= 2);
    auto *root = quick->rootObject();
    QVERIFY(QMetaObject::invokeMethod(root, "showOptions", Q_ARG(QVariant, QVariant(true))));
    auto *settings = root->findChild<QObject *>("settingsPage");
    QVERIFY(settings);
    const auto before = controller->settings(true);
    controller->SetUiLanguage("it");
    QTest::qWait(100);
    QCOMPARE(QCoreApplication::translate("Library", "Library"), QString("Libreria"));
    QCOMPARE(controller->settings(true), before);
    QCOMPARE(settings->property("category").toString(), QString("Library"));
    QSettings saved(controller->SettingsFile(), QSettings::IniFormat);
    QCOMPARE(saved.value("MainDialog/ui_language").toString(), QString("it"));
    const auto output = QCoreApplication::applicationDirPath() + "/library-qa";
    QVERIFY(quick->grabFramebuffer().save(output + "/italian-settings.png"));
    settings->setProperty("category", "Graphics");
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/italian-graphics.png"));
    root->setProperty("options", false);
    window.resize(980, 640);
    root->setProperty("consoleOpen", true);
    QTest::qWait(100);
    QVERIFY(quick->grabFramebuffer().save(output + "/italian-library.png"));
    UiTranslations::Instance().SetLanguage("en");
    MainDialog::ReadSettings(saved);
    QCOMPARE(UiTranslations::Instance().Language(), QString("it"));
    controller->SetUiLanguage("en");
    QCOMPARE(QCoreApplication::translate("Library", "Library"), QString("Library"));
    controller->SetUiLanguage("not-a-language");
    QCOMPARE(controller->UiLanguage(), QString("en"));
    window.close();
  }

  void cheatsCloseSafely() {
    LibraryController controller(nullptr);
    controller.selectGame(m_game);
    QVERIFY(controller.CanPatch());
    for (bool button : {false, true}) {
      bool closed = false;
      QTimer::singleShot(0, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        closed = true;
        if (button) {
          auto *buttons = dialog->findChild<QDialogButtonBox *>();
          for (auto *entry : buttons->buttons())
            if (buttons->buttonRole(entry) == QDialogButtonBox::RejectRole) entry->click();
        } else dialog->close();
      });
      controller.action("patches");
      QVERIFY(closed);
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
  }
  void userSettingsSurviveRestart() {
    QVERIFY(QFile::rename("Kyty.ini", "portable-backup.ini"));
    QString savedPath;
    {
      LibraryController controller(nullptr);
      savedPath = controller.SettingsFile();
      QVERIFY(savedPath.startsWith(m_directory.filePath("user")));
      controller.SetUiLanguage("it");
      QVERIFY(controller.saveSettings(true, {{"console_language", 5}, {"user_name", "Persistence test"}}).isEmpty());
    }
    UiTranslations::Instance().SetLanguage("en");
    {
      LibraryController restored(nullptr);
      QCOMPARE(restored.SettingsFile(), savedPath);
      QCOMPARE(restored.UiLanguage(), QString("it"));
      QVariantMap values;
      for (const auto &field : restored.settings(true))
        values.insert(field.toMap()["key"].toString(), field.toMap()["value"]);
      QCOMPARE(values["console_language"].toInt(), 5);
      QCOMPARE(values["user_name"].toString(), QString("Persistence test"));
    }
    QVERIFY(QFile::rename("portable-backup.ini", "Kyty.ini"));
    UiTranslations::Instance().SetLanguage("en");
  }
  void translatedControlsKeepBindingTokens() {
    UiTranslations::Instance().SetLanguage("it");
    const QStringList mapping{"Up=Up", "L3=Left Shift", "Circle=Mouse:Right"};
    InputMappingDialog dialog(mapping);
    auto *tree = dialog.findChild<QTreeWidget *>();
    QVERIFY(tree);
    QCOMPARE(tree->topLevelItem(0)->text(0), QString("Croce direzionale: su"));
    QCOMPARE(tree->topLevelItem(0)->text(1), QString("Freccia su"));
    auto actual = dialog.Mapping();
    auto expected = mapping;
    actual.sort(); expected.sort();
    QCOMPARE(actual, expected);
    UiTranslations::Instance().SetLanguage("en");
  }
  void compatibilityUpdatesGameStatus() {
    QFile file("compatibility_db.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"({"PPSA00001":{"status":"InGame","comment":"Test report","platforms":{"windows":{"status":"InGame"}}}})");
    file.close();
    {
      LibraryController controller(nullptr);
      controller.selectGame(m_game);
      QCOMPARE(controller.Selected()["status"].toString(), QString("In game"));
      QCOMPARE(controller.Selected()["comment"].toString(), QString("Test report"));
    }
    QVERIFY(file.remove());
  }
  void localizedTrophyMetadata() {
    const auto folder = m_directory.filePath("trophy-fixture/sce_sys/trophy2");
    QVERIFY(QDir().mkpath(folder));
    for (bool italianAvailable : {true, false}) {
      QList<QPair<QByteArray, QByteArray>> files{
        {"tropconf.json", R"({"defaultLanguage":"en-US","trophies":[{"id":0,"grade":"B"}]})"},
        {"tropmeta_en-US.json", R"({"metadata":{"trophyMetadata":[{"id":0,"name":"First step","detail":"Start the game"}]}})"}
      };
      if (italianAvailable)
        files.append(QPair<QByteArray, QByteArray>{"tropmeta_it-IT.json", R"({"metadata":{"trophyMetadata":[{"id":0,"name":"Primo passo","detail":"Avvia il gioco"}]}})"});
      QByteArray data(0x60 + files.size() * 0x40, '\0');
      qToBigEndian<quint32>(0xb228c60a, data.data());
      qToBigEndian<quint32>(1, data.data() + 4);
      qToBigEndian<quint32>(files.size(), data.data() + 0x10);
      qToBigEndian<quint32>(0x40, data.data() + 0x14);
      for (int i = 0; i < files.size(); ++i) {
        const auto offset = 0x60 + i * 0x40;
        data.replace(offset, files[i].first.size(), files[i].first);
        qToBigEndian<quint64>(data.size(), data.data() + offset + 0x20);
        qToBigEndian<quint64>(files[i].second.size(), data.data() + offset + 0x28);
        data.append(files[i].second);
      }
      qToBigEndian<quint64>(data.size(), data.data() + 8);
      QFile package(folder + "/trophy00.ucp");
      QVERIFY(package.open(QIODevice::WriteOnly));
      QCOMPARE(package.write(data), data.size());
      package.close();
      UiTranslations::Instance().SetLanguage("it");
      QString name, detail;
      QTimer::singleShot(0, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        auto *table = dialog->findChild<QTableWidget *>();
        if (table && table->rowCount()) {
          name = table->item(0, 2)->text(); detail = table->item(0, 3)->text();
        }
        dialog->reject();
      });
      Configuration info;
      info.basedir = m_directory.filePath("trophy-fixture");
      TrophyViewerDialog::ShowForGame(&info, nullptr);
      QCOMPARE(name, italianAvailable ? QString("Primo passo") : QString("First step"));
      QCOMPARE(detail, italianAvailable ? QString("Avvia il gioco") : QString("Start the game"));
    }
    UiTranslations::Instance().SetLanguage("en");
  }
  void cleanupTestCase() { QVERIFY(QDir::setCurrent(m_old_directory)); }
};

int main(int argc, char **argv) {
  // Use local compatibility fixtures and a portable INI; never touch user
  // preferences.
  char local[] = "--local";
  char *app_argv[]{argv[0], local, nullptr};
  int app_argc = 2;
  QApplication app(app_argc, app_argv);
  QQuickStyle::setStyle("Basic");
  LauncherTheme::Initialize(app);
  LibraryTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "library_ui_tests.moc"
