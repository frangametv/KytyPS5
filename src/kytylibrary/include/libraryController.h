#pragma once

#include <QAbstractListModel>
#include <QProcess>
#include <QSortFilterProxyModel>
#include <QStringDecoder>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class MainDialog;
class ConfigurationListWidget;
class UpdateChecker;
class Configuration;

class LibraryLogModel final: public QAbstractListModel {
	Q_OBJECT
public:
	explicit LibraryLogModel(QObject* parent = nullptr): QAbstractListModel(parent) {}
	int rowCount(const QModelIndex& parent = {}) const override {
		return parent.isValid() ? 0 : m_lines.size();
	}
	QVariant               data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override { return {{Qt::DisplayRole, "line"}}; }
	void                   Append(QStringList lines);
	void                   Clear();
	QString                Text() const { return m_lines.join('\n'); }

private:
	QStringList m_lines;
};

class LibraryController final: public QObject {
	Q_OBJECT
	Q_PROPERTY(QString uiLanguage READ UiLanguage WRITE SetUiLanguage NOTIFY uiLanguageChanged)
	Q_PROPERTY(QVariantList uiLanguages READ UiLanguages CONSTANT)
	Q_PROPERTY(QVariantList games READ Games NOTIFY libraryChanged)
	Q_PROPERTY(QVariantMap selected READ Selected NOTIFY selectionChanged)
	Q_PROPERTY(QStringList folders READ Folders NOTIFY libraryChanged)
	Q_PROPERTY(bool running READ Running NOTIFY stateChanged)
	Q_PROPERTY(bool stopping READ Stopping NOTIFY stateChanged)
	Q_PROPERTY(bool ready READ Ready NOTIFY stateChanged)
	Q_PROPERTY(bool scanning READ Scanning NOTIFY stateChanged)
	Q_PROPERTY(QString status READ Status NOTIFY stateChanged)
	Q_PROPERTY(QString version READ Version NOTIFY stateChanged)
	Q_PROPERTY(QString settingsFile READ SettingsFile CONSTANT)
	Q_PROPERTY(bool updatesSupported READ UpdatesSupported CONSTANT)
	Q_PROPERTY(bool checkUpdates READ CheckUpdates WRITE SetCheckUpdates NOTIFY stateChanged)
	Q_PROPERTY(bool localCompatibility READ LocalCompatibility CONSTANT)
	Q_PROPERTY(bool canViewTrophies READ CanViewTrophies NOTIFY selectionChanged)
	Q_PROPERTY(bool canPatch READ CanPatch NOTIFY selectionChanged)
	Q_PROPERTY(QAbstractItemModel* logModel READ LogModel CONSTANT)
public:
	explicit LibraryController(MainDialog* window);
	~LibraryController() override;
	void                Initialize(const QString& executablePath = {});
	QVariantList        Games() const;
	QVariantMap         Selected() const;
	QStringList         Folders() const;
	bool                Running() const { return m_process.state() != QProcess::NotRunning; }
	bool                Stopping() const { return m_stopping; }
	bool                Ready() const { return m_ready; }
	bool                Scanning() const { return m_scanning; }
	QString             Status() const { return m_status; }
	QString             Version() const { return m_version; }
	QString             UiLanguage() const;
	QVariantList        UiLanguages() const;
	void                SetUiLanguage(const QString& code);
	QString             SettingsFile() const;
	bool                UpdatesSupported() const;
	bool                CheckUpdates() const;
	void                SetCheckUpdates(bool checked);
	bool                LocalCompatibility() const;
	bool                CanViewTrophies() const;
	bool                CanPatch() const;
	QAbstractItemModel* LogModel() { return &m_log_filter; }
	bool                RequestClose();
	void                SaveGeometry();

	Q_INVOKABLE void         selectGame(const QString& path);
	Q_INVOKABLE void         start();
	Q_INVOKABLE void         stop();
	Q_INVOKABLE void         rescan();
	Q_INVOKABLE void         addFolder();
	Q_INVOKABLE void         removeFolder(int index);
	Q_INVOKABLE void         action(const QString& name);
	Q_INVOKABLE QVariantList settings(bool global) const;
	Q_INVOKABLE QVariantList defaults() const;
	Q_INVOKABLE QString      saveSettings(bool global, const QVariantMap& values);
	Q_INVOKABLE void         setCompatibility(int status, const QString& comment);
	Q_INVOKABLE void         clearLog();
	Q_INVOKABLE void         copyLog();
	Q_INVOKABLE void         exportLog();
	Q_INVOKABLE void         filterLog(const QString& text);
signals:
	void uiLanguageChanged();
	void libraryChanged();
	void selectionChanged();
	void stateChanged();
	void showConsole();
	void logUpdated();

private:
	void                     Refresh();
	void                     Drain(bool final = false);
	void                     Note(const QString& text);
	void                     Finish(int code, QProcess::ExitStatus status);
	MainDialog*              m_window;
	ConfigurationListWidget* m_library;
	UpdateChecker*           m_updates;
	QProcess                 m_process;
	QProcess                 m_probe;
	QTimer                   m_flush;
	QTimer                   m_stop_timer;
	LibraryLogModel          m_log;
	QSortFilterProxyModel    m_log_filter;
	QStringDecoder           m_decoder {QStringDecoder::Utf8};
	QString                  m_pending;
	QString                  m_interpreter;
	QString                  m_selected_path;
	QString                  m_running_path;
	QString                  m_status = "Locating emulator…";
	QString                  m_version;
	bool                     m_ready               = false;
	bool                     m_scanning            = false;
	bool                     m_stopping            = false;
	bool                     m_close_when_finished = false;
};

QStringList CreateEmulatorArgs(const Configuration& info);
