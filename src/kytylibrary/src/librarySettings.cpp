#include "librarySettings.h"

#include "configuration.h"

namespace {
const QStringList languages {"Japanese",
                             "English (United States)",
                             "French (France)",
                             "Spanish (Spain)",
                             "German",
                             "Italian",
                             "Dutch",
                             "Portuguese (Portugal)",
                             "Russian",
                             "Korean",
                             "Chinese (Traditional)",
                             "Chinese (Simplified)",
                             "Finnish",
                             "Swedish",
                             "Danish",
                             "Norwegian",
                             "Polish",
                             "Portuguese (Brazil)",
                             "English (United Kingdom)",
                             "Turkish",
                             "Spanish (Latin America)",
                             "Arabic",
                             "French (Canada)",
                             "Czech",
                             "Hungarian",
                             "Greek",
                             "Romanian",
                             "Thai",
                             "Vietnamese",
                             "Indonesian"};
}

QVariantList LibrarySettings::Fields(const Configuration& info) {
	QVariantList fields;
	fields.append(QVariantMap {{"key", "user_name"},
	                           {"label", "User name"},
	                           {"section", "System"},
	                           {"kind", "text"},
	                           {"value", info.user_name},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "user_id"},
	                           {"label", "User ID"},
	                           {"section", "System"},
	                           {"kind", "number"},
	                           {"value", info.user_id},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 2147483647}});
	fields.append(QVariantMap {{"key", "console_language"},
	                           {"label", "Console language"},
	                           {"section", "System"},
	                           {"kind", "choiceIndex"},
	                           {"value", info.console_language},
	                           {"choices", languages},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "screen_resolution"},
	                           {"label", "Resolution"},
	                           {"section", "Graphics"},
	                           {"kind", "enum"},
	                           {"value", EnumToText(info.screen_resolution)},
	                           {"choices", EnumToList<Configuration::Resolution>()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "present_mode"},
	                           {"label", "Presentation mode"},
	                           {"section", "Graphics"},
	                           {"kind", "enum"},
	                           {"value", EnumToText(info.present_mode)},
	                           {"choices", EnumToList<Configuration::PresentMode>()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "fullscreen_enabled"},
	                           {"label", "Start in fullscreen"},
	                           {"section", "Graphics"},
	                           {"kind", "bool"},
	                           {"value", info.fullscreen_enabled},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "vblank_frequency"},
	                           {"label", "Vblank frequency (Hz)"},
	                           {"section", "Graphics"},
	                           {"kind", "number"},
	                           {"value", info.vblank_frequency},
	                           {"choices", QStringList()},
	                           {"minimum", 30},
	                           {"maximum", 360}});
	fields.append(QVariantMap {{"key", "shader_optimization_type"},
	                           {"label", "Shader optimization"},
	                           {"section", "Graphics"},
	                           {"kind", "enum"},
	                           {"value", EnumToText(info.shader_optimization_type)},
	                           {"choices", EnumToList<Configuration::ShaderOptimizationType>()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "vulkan_validation_enabled"},
	                           {"label", "Vulkan validation"},
	                           {"section", "Advanced"},
	                           {"kind", "bool"},
	                           {"value", info.vulkan_validation_enabled},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "shader_validation_enabled"},
	                           {"label", "Shader validation"},
	                           {"section", "Advanced"},
	                           {"kind", "bool"},
	                           {"value", info.shader_validation_enabled},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "renderdoc_enabled"},
	                           {"label", "RenderDoc capture"},
	                           {"section", "Advanced"},
	                           {"kind", "bool"},
	                           {"value", info.renderdoc_enabled},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
#if defined(_WIN32)
	fields.append(QVariantMap {{"key", "red_zone_protection_enabled"},
	                           {"label", "Red zone protection"},
	                           {"section", "Advanced"},
	                           {"kind", "bool"},
	                           {"value", info.red_zone_protection_enabled},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
#endif
	fields.append(QVariantMap {{"key", "shader_log_direction"},
	                           {"label", "Shader logging"},
	                           {"section", "Logging"},
	                           {"kind", "enum"},
	                           {"value", EnumToText(info.shader_log_direction)},
	                           {"choices", EnumToList<Configuration::ShaderLogDirection>()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "shader_log_folder"},
	                           {"label", "Shader log folder"},
	                           {"section", "Logging"},
	                           {"kind", "text"},
	                           {"value", info.shader_log_folder},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "command_buffer_dump_enabled"},
	                           {"label", "Dump command buffers"},
	                           {"section", "Logging"},
	                           {"kind", "bool"},
	                           {"value", info.command_buffer_dump_enabled},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "command_buffer_dump_folder"},
	                           {"label", "Command buffer folder"},
	                           {"section", "Logging"},
	                           {"kind", "text"},
	                           {"value", info.command_buffer_dump_folder},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "printf_direction"},
	                           {"label", "Emulator log output"},
	                           {"section", "Logging"},
	                           {"kind", "enum"},
	                           {"value", EnumToText(info.printf_direction)},
	                           {"choices", EnumToList<Configuration::LogDirection>()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "printf_output_file"},
	                           {"label", "Emulator log file"},
	                           {"section", "Logging"},
	                           {"kind", "text"},
	                           {"value", info.printf_output_file},
	                           {"choices", QStringList()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	fields.append(QVariantMap {{"key", "profiler_direction"},
	                           {"label", "Profiler output"},
	                           {"section", "Logging"},
	                           {"kind", "enum"},
	                           {"value", EnumToText(info.profiler_direction)},
	                           {"choices", EnumToList<Configuration::ProfilerDirection>()},
	                           {"minimum", 0},
	                           {"maximum", 0}});
	return fields;
}

QString LibrarySettings::Apply(Configuration& info, const QVariantMap& values) {
	if (values.contains("user_name")) {
		info.user_name = values.value("user_name").toString();
	}
	if (values.contains("user_id")) {
		bool      ok    = false;
		const int value = values.value("user_id").toInt(&ok);
		if (!ok || value < 0 || value > 2147483647) return "Invalid user id";
		info.user_id = value;
	}
	if (values.contains("console_language")) {
		bool      ok    = false;
		const int value = values.value("console_language").toInt(&ok);
		if (!ok || value < 0 || value > 29) return "Invalid console language";
		info.console_language = value;
	}
	if (values.contains("screen_resolution")) {
		const auto text = values.value("screen_resolution").toString();
		if (!EnumToList<Configuration::Resolution>().contains(text)) return "Invalid resolution";
		info.screen_resolution = TextToEnum<Configuration::Resolution>(text);
	}
	if (values.contains("present_mode")) {
		const auto text = values.value("present_mode").toString();
		if (!EnumToList<Configuration::PresentMode>().contains(text))
			return "Invalid presentation mode";
		info.present_mode = TextToEnum<Configuration::PresentMode>(text);
	}
	if (values.contains("fullscreen_enabled")) {
		info.fullscreen_enabled = values.value("fullscreen_enabled").toBool();
	}
	if (values.contains("vblank_frequency")) {
		bool      ok    = false;
		const int value = values.value("vblank_frequency").toInt(&ok);
		if (!ok || value < 30 || value > 360) return "Invalid vblank frequency (hz)";
		info.vblank_frequency = value;
	}
	if (values.contains("shader_optimization_type")) {
		const auto text = values.value("shader_optimization_type").toString();
		if (!EnumToList<Configuration::ShaderOptimizationType>().contains(text))
			return "Invalid shader optimization";
		info.shader_optimization_type = TextToEnum<Configuration::ShaderOptimizationType>(text);
	}
	if (values.contains("vulkan_validation_enabled")) {
		info.vulkan_validation_enabled = values.value("vulkan_validation_enabled").toBool();
	}
	if (values.contains("shader_validation_enabled")) {
		info.shader_validation_enabled = values.value("shader_validation_enabled").toBool();
	}
	if (values.contains("renderdoc_enabled")) {
		info.renderdoc_enabled = values.value("renderdoc_enabled").toBool();
	}
#if defined(_WIN32)
	if (values.contains("red_zone_protection_enabled")) {
		info.red_zone_protection_enabled = values.value("red_zone_protection_enabled").toBool();
	}
#endif
	if (values.contains("shader_log_direction")) {
		const auto text = values.value("shader_log_direction").toString();
		if (!EnumToList<Configuration::ShaderLogDirection>().contains(text))
			return "Invalid shader logging";
		info.shader_log_direction = TextToEnum<Configuration::ShaderLogDirection>(text);
	}
	if (values.contains("shader_log_folder")) {
		info.shader_log_folder = values.value("shader_log_folder").toString();
	}
	if (values.contains("command_buffer_dump_enabled")) {
		info.command_buffer_dump_enabled = values.value("command_buffer_dump_enabled").toBool();
	}
	if (values.contains("command_buffer_dump_folder")) {
		info.command_buffer_dump_folder = values.value("command_buffer_dump_folder").toString();
	}
	if (values.contains("printf_direction")) {
		const auto text = values.value("printf_direction").toString();
		if (!EnumToList<Configuration::LogDirection>().contains(text))
			return "Invalid emulator log output";
		info.printf_direction = TextToEnum<Configuration::LogDirection>(text);
	}
	if (values.contains("printf_output_file")) {
		info.printf_output_file = values.value("printf_output_file").toString();
	}
	if (values.contains("profiler_direction")) {
		const auto text = values.value("profiler_direction").toString();
		if (!EnumToList<Configuration::ProfilerDirection>().contains(text))
			return "Invalid profiler output";
		info.profiler_direction = TextToEnum<Configuration::ProfilerDirection>(text);
	}
	info.user_name = info.user_name.trimmed();
	if (info.user_name.isEmpty() || info.user_name.toUtf8().size() > Config::MAX_USER_NAME_LENGTH)
		return "User name must contain 1–16 UTF-8 bytes.";
	if (!Config::IsConfiguredUserIdValid(info.user_id))
		return "User ID cannot be 254 (everyone) or 255 (system).";
	if (info.shader_log_direction == Configuration::ShaderLogDirection::File &&
	    info.shader_log_folder.trimmed().isEmpty())
		return "Choose a shader log folder.";
	if (info.command_buffer_dump_enabled && info.command_buffer_dump_folder.trimmed().isEmpty())
		return "Choose a command buffer folder.";
	if (info.printf_direction == Configuration::LogDirection::File &&
	    info.printf_output_file.trimmed().isEmpty())
		return "Choose a emulator log output file.";
	return {};
}
