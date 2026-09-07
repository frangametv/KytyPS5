#include "libraryController.h"

#include "configuration.h"
#include "configurationItem.h"
#include "configurationListWidget.h"
#include "librarySettings.h"
#include "uiTranslations.h"
#include "kytyGitVersion.h"
#include "mainDialog.h"
#include "patchesDialog.h"
#include "updateChecker.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrl>

#if defined(_WIN32)
#include <windows.h>
#endif

QVariant LibraryLogModel::data(const QModelIndex& index, int role) const {
	return index.isValid() && index.row() >= 0 && index.row() < m_lines.size() &&
	               role == Qt::DisplayRole
	           ? QVariant(m_lines.at(index.row()))
	           : QVariant();
}

void LibraryLogModel::Append(QStringList lines) {
	constexpr int limit = 6000;
	if (lines.isEmpty()) return;
	if (lines.size() > limit) lines = lines.last(limit);
	const int remove = qMax(0, static_cast<int>(m_lines.size() + lines.size()) - limit);
	if (remove > 0) {
		beginRemoveRows({}, 0, remove - 1);
		m_lines.erase(m_lines.begin(), m_lines.begin() + remove);
		endRemoveRows();
	}
	beginInsertRows({}, m_lines.size(), m_lines.size() + lines.size() - 1);
	m_lines.append(lines);
	endInsertRows();
}

void LibraryLogModel::Clear() {
	beginResetModel();
	m_lines.clear();
	endResetModel();
}

LibraryController::LibraryController(MainDialog* window)
    : QObject(window), m_window(window), m_library(new ConfigurationListWidget(window)),
      m_updates(new UpdateChecker(window)) {
	m_library->hide();
	m_library->SetMainDialog(window);
	m_log_filter.setSourceModel(&m_log);
	m_log_filter.setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(m_library, &ConfigurationListWidget::LibraryChanged, this, &LibraryController::Refresh);
	connect(m_library, &ConfigurationListWidget::Run, this, &LibraryController::start);
	connect(&m_process, &QProcess::stateChanged, this, [this] { emit stateChanged(); });
	connect(&m_process, &QProcess::started, this, [this] {
		m_status = "Running";
		Note("Emulation started.");
		emit stateChanged();
	});
	connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
		Note("Process error: " + m_process.errorString());
		if (error == QProcess::FailedToStart) {
			m_flush.stop();
			m_status = "Unable to start emulator";
			m_running_path.clear();
			m_stopping = false;
			m_stop_timer.stop();
			emit stateChanged();
			if (m_close_when_finished && m_window) {
				m_close_when_finished = false;
				QTimer::singleShot(0, m_window, &QWidget::close);
			}
		}
	});
	connect(&m_process, &QProcess::finished, this, &LibraryController::Finish);
	m_process.setProcessChannelMode(QProcess::MergedChannels);
#if defined(_WIN32)
	m_process.setCreateProcessArgumentsModifier(
	    [](QProcess::CreateProcessArguments* args) { args->flags |= CREATE_NO_WINDOW; });
#endif
	m_flush.setInterval(80);
	connect(&m_flush, &QTimer::timeout, this, [this] { Drain(); });
	m_stop_timer.setSingleShot(true);
	m_stop_timer.setInterval(3000);
	connect(&m_stop_timer, &QTimer::timeout, this, [this] {
		if (Running()) {
			Note("The emulator did not exit; stopping the process.");
			m_process.kill();
		}
	});
}

LibraryController::~LibraryController() {
	m_process.disconnect(this);
	m_probe.disconnect(this);
	m_probe.kill();
	m_probe.waitForFinished(1000);
	if (Running()) {
		m_process.kill();
		m_process.waitForFinished(3000);
	}
	delete m_library;
}

void LibraryController::Initialize(const QString& executablePath) {
	m_version = QStringLiteral(KYTY_GIT_VERSION);
#if defined(_WIN32)
	const QString executable = "kyty_emulator.exe";
#else
	const QString executable = "kyty_emulator";
#endif
	QDir directory(QApplication::applicationDirPath());
	m_interpreter = directory.filePath(executable);
	if (!QFileInfo::exists(m_interpreter)) {
		directory.cdUp();
		m_interpreter = directory.filePath(executable);
	}
	if (!executablePath.isEmpty()) m_interpreter = executablePath;
	Refresh();
	if (!m_selected_path.isEmpty()) selectGame(m_selected_path);
	if (!QFileInfo::exists(m_interpreter)) {
		m_status = "Emulator not found";
		Note("Place " + executable + " next to kyty_library.");
		emit stateChanged();
		return;
	}
	connect(&m_probe, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exit) {
		const auto lines = QString::fromUtf8(m_probe.readAllStandardOutput())
		                       .split(QRegularExpression("[\\r\\n]"), Qt::SkipEmptyParts);
		m_ready          = code == 0 && exit == QProcess::NormalExit && !lines.isEmpty();
		m_status         = m_ready ? "Emulator ready" : "Emulator check failed";
		if (!m_ready) Note(m_status + ". Check the emulator installation.");
		emit stateChanged();
		if (m_ready && CheckUpdates() && !QCoreApplication::arguments().contains("--local"))
			m_updates->Check(false);
	});
	connect(&m_probe, &QProcess::errorOccurred, this, [this] {
		m_status = "Cannot open emulator";
		Note(m_probe.errorString());
		emit stateChanged();
	});
	m_probe.start(m_interpreter, {});
	QTimer::singleShot(10000, &m_probe, [this] {
		if (m_probe.state() != QProcess::NotRunning) m_probe.kill();
	});
}

QVariantList LibraryController::Games() const {
	return m_library->LibraryEntries();
}
QStringList LibraryController::Folders() const {
	return m_library->GetGameDirectories();
}
QString LibraryController::SettingsFile() const {
	return m_library->GetSettingsFile();
}
bool LibraryController::UpdatesSupported() const {
	return UpdateChecker::IsSupported();
}
bool LibraryController::CheckUpdates() const {
	return MainDialog::CheckUpdatesOnStartup();
}
bool LibraryController::LocalCompatibility() const {
	return m_library->IsLocalCompatibility();
}
bool LibraryController::CanViewTrophies() const {
	return m_library->CanViewSelectedTrophies();
}
bool LibraryController::CanPatch() const {
	const auto* item = m_library->GetSelectedItem();
	return item && PatchesDialog::IsSupportedTitleId(item->GetInfo().title_id);
}

void LibraryController::SetCheckUpdates(bool checked) {
	MainDialog::SetCheckUpdatesOnStartup(checked);
	m_library->WriteSettings();
	emit stateChanged();
}

QVariantMap LibraryController::Selected() const {
	for (const auto& entry: Games()) {
		if (entry.toMap().value("path").toString() == m_selected_path) return entry.toMap();
	}
	return {};
}

void LibraryController::Refresh() {
	const auto games = Games();
	bool       found = false;
	for (const auto& entry: games)
		found |= entry.toMap().value("path").toString() == m_selected_path;
	if (!found)
		m_selected_path =
		    games.isEmpty() ? QString() : games.first().toMap().value("path").toString();
	m_library->SelectPath(m_selected_path);
	emit libraryChanged();
	emit selectionChanged();
}

void LibraryController::selectGame(const QString& path) {
	m_library->SelectPath(path);
	const auto* item = m_library->GetSelectedItem();
	m_selected_path  = item ? item->GetInfo().game_path : QString();
	emit selectionChanged();
}

void LibraryController::start() {
	if (Running() || !m_ready || m_scanning) return;
	const auto* item = m_library->GetSelectedItem();
	if (!item) return;
	Configuration info;
	info.CopyFrom(item->GetInfo());
	info.host_input_mapping = m_library->GetHostInputMapping();
	const auto args         = CreateEmulatorArgs(info);
	emit       showConsole();
	if (args.isEmpty() || !QFileInfo::exists(QDir(info.basedir).filePath(info.elf))) {
		Note("Cannot start: the game executable is missing or the configuration is invalid.");
		return;
	}
	m_decoder.resetState();
	m_pending.clear();
	m_stop_timer.stop();
	m_stopping     = false;
	m_running_path = info.game_path;
	m_status       = "Starting " + info.name;
	Note("Starting " + info.name + " [" + info.title_id + "]");
	if (info.printf_direction == Configuration::LogDirection::Silent)
		Note("Emulator logging is Silent. Select Console under Options > Logging for detailed "
		     "output.");
	auto environment = QProcessEnvironment::systemEnvironment();
	environment.insert("KYTY_LIBRARY_STDOUT", "1");
	m_process.setProcessEnvironment(environment);
	m_process.setWorkingDirectory(QFileInfo(m_interpreter).absolutePath());
	m_process.start(m_interpreter, args);
	m_flush.start();
	emit stateChanged();
}

void LibraryController::stop() {
	if (!Running() || m_stopping) return;
	m_stopping = true;
	m_status   = "Stopping…";
	Note("Stop requested.");
	m_process.terminate();
	m_stop_timer.start();
	emit stateChanged();
}

void LibraryController::Finish(int code, QProcess::ExitStatus exit) {
	m_flush.stop();
	m_stop_timer.stop();
	Drain(true);
	m_status = m_stopping                    ? "Stopped"
	           : exit == QProcess::CrashExit ? "Emulator crashed"
	           : code == 0                   ? "Session finished"
	                                         : "Emulator exited with an error";
	Note(QString("%1 (exit code %2).").arg(m_status).arg(code));
	m_stopping = false;
	m_running_path.clear();
	emit stateChanged();
	if (m_close_when_finished && m_window) {
		m_close_when_finished = false;
		QTimer::singleShot(0, m_window, &QWidget::close);
	}
}

void LibraryController::Drain(bool final) {
	// Decode across reads so split UTF-8 characters survive redirected output.
	const auto bytes = m_process.readAllStandardOutput();
	m_pending += m_decoder(bytes);
	if (m_pending.size() > 1024 * 1024) {
		m_pending = m_pending.right(1024 * 1024);
		Note("Log burst truncated to keep the library responsive.");
	}
	int end = final ? m_pending.size() : m_pending.lastIndexOf('\n') + 1;
	if (end == 0 && m_pending.size() > 16384) end = m_pending.size();
	if (end <= 0) return;
	QString text = m_pending.left(end);
	m_pending.remove(0, end);
	static const QRegularExpression ansi("\\x1b\\[[0-?]*[ -/]*[@-~]|\\x1b\\][^\\x07]*(?:\\x07)");
	text.remove(ansi);
	text.replace("\r\n", "\n");
	text.replace('\r', '\n');
	auto lines = text.split('\n');
	if (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();
	for (auto& line: lines)
		if (line.size() > 16384) line = line.left(16384) + " …";
	m_log.Append(lines);
	emit logUpdated();
}

void LibraryController::Note(const QString& text) {
	m_log.Append({QString("[Library] %1").arg(text)});
	emit logUpdated();
}

void LibraryController::rescan() {
	if (Running() || m_scanning) return;
	m_scanning = true;
	emit stateChanged();
	QTimer::singleShot(0, this, [this] {
		m_library->ScanGameDirectory();
		m_scanning = false;
		emit stateChanged();
	});
}

void LibraryController::addFolder() {
	if (Running() || m_scanning) return;
	const auto path = QFileDialog::getExistingDirectory(m_window, tr("Add game folder"));
	if (path.isEmpty()) return;
	auto folders = Folders();
	folders.append(path);
	m_library->SetGameDirectories(folders);
}

void LibraryController::removeFolder(int index) {
	if (Running() || m_scanning) return;
	auto folders = Folders();
	if (index < 0 || index >= folders.size()) return;
	folders.removeAt(index);
	m_library->SetGameDirectories(folders);
}

QVariantList LibraryController::settings(bool global) const {
	const auto* item = m_library->GetSelectedItem();
	return LibrarySettings::Fields(global || !item ? m_library->GlobalConfiguration()
	                                               : item->GetInfo());
}

QVariantList LibraryController::defaults() const {
	return LibrarySettings::Fields(Configuration());
}

QString LibraryController::saveSettings(bool global, const QVariantMap& values) {
	if (Running()) return "Stop the game before changing settings.";
	if (!global && !m_library->GetSelectedItem()) return "Select a game first.";
	Configuration info;
	info.CopyFrom(global ? m_library->GlobalConfiguration()
	                     : m_library->GetSelectedItem()->GetInfo());
	const auto error = LibrarySettings::Apply(info, values);
	if (!error.isEmpty()) return error;
	m_library->SaveConfiguration(info, global);
	Note(global ? "Global settings saved." : "Game settings saved.");
	return {};
}

void LibraryController::setCompatibility(int status, const QString& comment) {
	m_library->SetCompatibility(status, comment);
}

void LibraryController::action(const QString& name) {
	if (name == "updates") {
		m_updates->Check(true);
		return;
	}
	if (name == "openSettings") {
		QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(SettingsFile()).absolutePath()));
		return;
	}
	if (name == "input") {
		if (!Running()) QMetaObject::invokeMethod(m_library, "edit_input_mapping");
		return;
	}
	const auto* item = m_library->GetSelectedItem();
	if (!item) return;
	if (name == "folder")
		QMetaObject::invokeMethod(m_library, "open_game_folder");
	else if (name == "copyPath")
		QApplication::clipboard()->setText(item->GetInfo().basedir);
	else if (name == "copyTitleId")
		QApplication::clipboard()->setText(item->GetInfo().title_id);
	else if (name == "trophies")
		m_library->ViewTrophies();
	else if (!Running()) {
		if (name == "reset") {
			QMetaObject::invokeMethod(m_library, "delete_configuartion");
			Refresh();
		} else if (name == "saves")
			QMetaObject::invokeMethod(m_library, "remove_save_data");
		else if (name == "patches" && CanPatch()) {
			PatchesDialog dialog(item->GetInfo(), m_window);
			dialog.exec();
		}
	}
}

void LibraryController::clearLog() {
	m_log.Clear();
	emit logUpdated();
}
void LibraryController::copyLog() {
	QApplication::clipboard()->setText(m_log.Text());
}
void LibraryController::filterLog(const QString& text) {
	m_log_filter.setFilterFixedString(text);
}
void LibraryController::exportLog() {
	const auto path = QFileDialog::getSaveFileName(m_window, tr("Export console"), "kyty-session.log",
	                                               "Log files (*.log *.txt)");
	if (path.isEmpty()) return;
	QSaveFile  file(path);
	const auto bytes = m_log.Text().toUtf8();
	if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
		QMessageBox::warning(m_window, tr("Export failed"), file.errorString());
}

bool LibraryController::RequestClose() {
	if (!Running()) return true;
	if (QMessageBox::question(m_window, tr("Close KytyPS5"),
	                          tr("Stop the running game and close the library?")) != QMessageBox::Yes)
		return false;
	m_close_when_finished = true;
	stop();
	return false;
}

void LibraryController::SaveGeometry() {
	m_library->WriteSettings();
}

QString LibraryController::UiLanguage() const { return UiTranslations::Instance().Language(); }
QVariantList LibraryController::UiLanguages() const { return UiTranslations::Instance().Languages(); }
void LibraryController::SetUiLanguage(const QString& code) {
    if (!UiTranslations::Instance().SetLanguage(code)) return;
    m_library->WriteSettings();
    emit uiLanguageChanged();
}
