/* Copyright 2015 Matt Wilmink */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcess>

#include "./app.h"

enum ErrorTypes {
  InvalidJSON,
  MissingObject,
  MissingArray
};

enum CommandLineParseResult {
  CommandLineOk,
  CommandLineError,
  CommandLineVersionRequested,
  CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser *parser,
                                        QString *recipe_file,
                                        QString *error_message) {
  const QCommandLineOption helpOption = parser->addHelpOption();
  const QCommandLineOption versionOption = parser->addVersionOption();
  parser->addPositionalArgument("recipe", qApp->tr("JSON recipe file to use"));

  if (!parser->parse(QCoreApplication::arguments())) {
    *error_message = parser->errorText();
    return CommandLineError;
  }

  if (parser->isSet(versionOption))
    return CommandLineVersionRequested;

  if (parser->isSet(helpOption))
    return CommandLineHelpRequested;

  const QStringList args = parser->positionalArguments();

  if (args.isEmpty()) {
    *error_message = "Argument 'recipe' missing.";
    return CommandLineError;
  }

  *recipe_file = args.at(0);
  return CommandLineOk;
}

QJsonDocument loadRecipe(QString recipe_path) {
  QFile recipe_json(recipe_path);
  if (recipe_json.open(QFile::ReadOnly))
    return QJsonDocument().fromJson(recipe_json.readAll());
  else
    return QJsonDocument();
}

bool isGlob(QString target) {
  return target.contains("*");
}

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QCommandLineParser parser;

  QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
  QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);
  QCoreApplication::setOrganizationName(VER_COMPANYNAME_STR);
  QCoreApplication::setOrganizationDomain(VER_COMPANYDOMAIN_STR);

  parser.setApplicationDescription(VER_FILEDESCRIPTION_STR);

  QString error_message;
  QString recipe_file;
  switch (parseCommandLine(&parser, &recipe_file, &error_message)) {
    case CommandLineOk:
      break;
    case CommandLineError:
      fputs(qPrintable(error_message), stderr);
      fputs("\n\n", stderr);
      fputs(qPrintable(parser.helpText()), stderr);
      return 1;
    case CommandLineVersionRequested:
      printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
             qPrintable(QCoreApplication::applicationVersion()));
      return 0;
    case CommandLineHelpRequested:
      parser.showHelp();
      Q_UNREACHABLE();
  }

  QJsonDocument recipe = loadRecipe(recipe_file);

  if (!recipe.isObject()) {
    const QString err = qApp->tr("Invalid JSON obect define by %1. Aborting.")
                        .arg(recipe_file);
    fputs(qPrintable(err), stderr);
    return InvalidJSON;
  }

  QJsonObject recipe_object = recipe.object();

  if (!recipe_object.contains("ingredients") ||
      !recipe_object.value("ingredients").isObject()) {
    const QString err = qApp->tr("Missing ingredient object. Aborting.");
    fputs(qPrintable(err), stderr);
    return MissingObject;
  }

  if (!recipe_object.contains("utensils") ||
      !recipe_object.value("utensils").isObject()) {
    const QString err = qApp->tr("Missing utensils object. Aborting.");
    fputs(qPrintable(err), stderr);
    return MissingObject;
  }

  if (!recipe_object.contains("instructions") ||
      !recipe_object.value("instructions").isArray()) {
    const QString err = qApp->tr("Missing in instructions array. Aborting.");
    fputs(qPrintable(err), stderr);
    return MissingArray;
  }

  QJsonObject ingredients  = recipe_object.value("ingredients").toObject();
  QJsonObject utensils     = recipe_object.value("utensils").toObject();
  QJsonArray  instructions = recipe_object.value("instructions").toArray();

  const QString invalid_instruction =
      qApp->tr("Instuction %1 is invalid. Skipping");

  for (int i = 0; i < instructions.count(); i++) {
    if (!instructions.at(i).isObject()) {
      fputs(qPrintable(invalid_instruction.arg(i + 1)), stderr);
      continue;
    }

    QJsonObject instruction = instructions.at(i).toObject();

    if (!instruction.contains("run") || !instruction.contains("on")) {
      fputs(qPrintable(invalid_instruction.arg(i + 1)), stderr);
      continue;
    }

    if (!instruction.value("run").isString() ||
        !instruction.value("on").isString()) {
      fputs(qPrintable(invalid_instruction.arg(i + 1)), stderr);
      continue;
    }

    QString utensil = instruction.value("run").toString();
    QString ingredient = instruction.value("on").toString();

    if (!utensils.contains(utensil)) {
      const QString error =
          qApp->tr("Missing utensil %1. Skipping current instruction");
      fputs(qPrintable(error.arg(utensil)), stderr);
      continue;
    }

    if (!ingredients.contains(ingredient)) {
      const QString error =
          qApp->tr("Missing ingredient %1. Skipping current instruction");
      fputs(qPrintable(error.arg(ingredient)), stderr);
      continue;
    }

    QString command = utensils.value(utensil).toString();
    QString target = ingredients.value(ingredient).toString();

    QDir files;

    if (isGlob(target)) {
      files.setNameFilters(target.split(" "));
    } else {
      if (!files.cd(target)) {
        // NOLINTNEXTLINE
        const QString error = qApp->tr("Missing directory %1. required for ingredient %2. Skipping current instruction");
        fputs(qPrintable(error.arg(target).arg(ingredient)), stderr);
        continue;
      }
    }

    QStringList fileList = files.entryList(QDir::Files);

    for (QString file : fileList) {
      QString run = command;

      if (!isGlob(target)) file = target + "/" + file;

      run = run.replace("%i", file);
      run = run.replace("%n" , file.left(file.lastIndexOf(".")));

      QProcess process;

      #ifdef Q_OS_WIN
        process.start("cmd /c" + run);
      #endif

      #ifndef Q_OS_WIN
        process.start("$SHELL -c" + run);
      #endif

      while (process.waitForFinished())
        qApp->processEvents();

      const QString result = qApp->tr("Instruction: %1 \r\n%2");
      QString process_log = QString(process.readAll());

      fputs(qPrintable(result.arg(run).arg(process_log)), stdout);
    }
  }
}
