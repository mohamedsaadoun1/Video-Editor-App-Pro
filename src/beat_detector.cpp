/**
 * @file beat_detector.cpp
 * @brief Implementation of beat detection functionality
 */

#include "beat_detector.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QVariantList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

BeatDetector::BeatDetector(QObject *parent) 
    : QObject(parent), m_aubioModule(nullptr)
{
    createTempDir();
    initializePython();
    
    // Connect process signals
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            emit processCompleted(true, "Process completed successfully");
        } else {
            emit processCompleted(false, "Process failed with exit code: " + QString::number(exitCode));
        }
    });
    
    connect(&m_process, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        emit processCompleted(false, "Process error: " + QString::number(error));
    });
}

BeatDetector::~BeatDetector()
{
    // Clean up Python
    if (m_aubioModule) {
        Py_DECREF(m_aubioModule);
    }
    Py_Finalize();
    
    cleanupTempFiles();
}

bool BeatDetector::initializePython()
{
    qDebug() << "Initializing Python interpreter for beat detection...";
    
    // Initialize the Python interpreter
    Py_Initialize();
    
    // Import aubio module
    try {
        PyObject* aubioName = PyUnicode_FromString("aubio");
        m_aubioModule = PyImport_Import(aubioName);
        Py_DECREF(aubioName);
        
        if (!m_aubioModule) {
            qWarning() << "Failed to import aubio module";
            PyErr_Print();
            return false;
        }
        
        qDebug() << "Aubio module successfully imported";
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in Python initialization:" << e.what();
        return false;
    }
}

void BeatDetector::createTempDir()
{
    // Create temp directory for processing
    m_tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/advanced_video_editor_beats";
    QDir dir;
    if (!dir.exists(m_tempDir)) {
        dir.mkpath(m_tempDir);
    }
}

QVector<BeatDetector::Beat> BeatDetector::detectBeats(const QString& audioFile, double threshold)
{
    qDebug() << "Detecting beats in:" << audioFile << "with threshold:" << threshold;
    emit progressUpdate(0.0, "Starting beat detection...");
    
    // Validate input file
    if (!QFileInfo::exists(audioFile)) {
        qWarning() << "Input audio file does not exist:" << audioFile;
        emit processCompleted(false, "Input audio file does not exist: " + audioFile);
        return QVector<Beat>();
    }
    
    // Try Python-based detection first, fall back to command-line if it fails
    QVector<Beat> beats = detectBeatsWithPython(audioFile, threshold);
    
    if (beats.isEmpty()) {
        qDebug() << "Falling back to command-line beat detection";
        beats = detectBeatsWithCommandLine(audioFile, threshold);
    }
    
    // Convert to QVariantList for signal
    QVariantList beatList;
    for (const Beat& beat : beats) {
        beatList.append(beat.time);
    }
    
    emit beatsDetected(beatList);
    emit progressUpdate(1.0, "Beat detection completed");
    emit processCompleted(true, QString("Detected %1 beats").arg(beats.size()));
    
    return beats;
}

QVariantList BeatDetector::getBeatTimes(const QString& audioFile, double threshold)
{
    QVector<Beat> beats = detectBeats(audioFile, threshold);
    
    QVariantList beatTimes;
    for (const Beat& beat : beats) {
        beatTimes.append(beat.time);
    }
    
    return beatTimes;
}

double BeatDetector::detectTempo(const QString& audioFile)
{
    qDebug() << "Detecting tempo in:" << audioFile;
    emit progressUpdate(0.0, "Starting tempo detection...");
    
    // Validate input file
    if (!QFileInfo::exists(audioFile)) {
        qWarning() << "Input audio file does not exist:" << audioFile;
        emit processCompleted(false, "Input audio file does not exist: " + audioFile);
        return 0.0;
    }
    
    // Create a Python script to detect tempo
    QString scriptPath = m_tempDir + "/detect_tempo.py";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "import sys\n";
        out << "import aubio\n\n";
        out << "def detect_tempo(input_file):\n";
        out << "    # Open the audio file\n";
        out << "    source = aubio.source(input_file)\n";
        out << "    samplerate = source.samplerate\n\n";
        out << "    # Create a tempo detection object\n";
        out << "    tempo = aubio.tempo('default', 1024, 512, samplerate)\n\n";
        out << "    # Storage for the running tempo estimates\n";
        out << "    tempi = []\n\n";
        out << "    # Process the audio\n";
        out << "    total_frames = source.duration\n";
        out << "    read_frames = 0\n\n";
        out << "    while True:\n";
        out << "        samples, read = source()\n";
        out << "        is_beat = tempo(samples)\n";
        out << "        if is_beat:\n";
        out << "            this_tempo = tempo.get_bpm()\n";
        out << "            tempi.append(this_tempo)\n\n";
        out << "        # Update progress\n";
        out << "        read_frames += read\n";
        out << "        if read < source.hop_size:\n";
        out << "            break\n\n";
        out << "    # Return the average tempo\n";
        out << "    if tempi:\n";
        out << "        return sum(tempi) / len(tempi)\n";
        out << "    else:\n";
        out << "        return 0.0\n\n";
        out << "if __name__ == '__main__':\n";
        out << "    if len(sys.argv) < 2:\n";
        out << "        print('Usage: python detect_tempo.py input_file')\n";
        out << "        sys.exit(1)\n\n";
        out << "    input_file = sys.argv[1]\n";
        out << "    tempo = detect_tempo(input_file)\n";
        out << "    print(f'TEMPO:{tempo}')\n";
        out << "    sys.exit(0)\n";
        scriptFile.close();
        
        // Execute the Python script
        QProcess process;
        process.start("python3", QStringList() << scriptPath << audioFile);
        if (!process.waitForStarted()) {
            qWarning() << "Failed to start Python tempo detection process";
            emit processCompleted(false, "Failed to start tempo detection process");
            return 0.0;
        }
        
        emit progressUpdate(0.5, "Processing audio...");
        
        process.waitForFinished(-1);
        
        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split("\n", Qt::SkipEmptyParts);
            
            for (const QString& line : lines) {
                if (line.startsWith("TEMPO:")) {
                    bool ok;
                    double tempo = line.mid(6).toDouble(&ok);
                    if (ok) {
                        emit progressUpdate(1.0, "Tempo detection completed");
                        emit processCompleted(true, QString("Detected tempo: %1 BPM").arg(tempo));
                        return tempo;
                    }
                }
            }
        }
        
        qWarning() << "Failed to detect tempo: " << process.readAllStandardError();
        emit processCompleted(false, "Failed to detect tempo");
        return 0.0;
    } else {
        qWarning() << "Failed to create Python script for tempo detection";
        emit processCompleted(false, "Failed to create Python script for tempo detection");
        return 0.0;
    }
}

QStringList BeatDetector::splitAtBeats(const QString& inputFile, const QString& outputDir, double threshold)
{
    qDebug() << "Splitting file at beats:" << inputFile << "with threshold:" << threshold;
    emit progressUpdate(0.0, "Starting beat-based splitting...");
    
    // Validate input file
    if (!QFileInfo::exists(inputFile)) {
        qWarning() << "Input file does not exist:" << inputFile;
        emit processCompleted(false, "Input file does not exist: " + inputFile);
        return QStringList();
    }
    
    // Ensure output directory exists
    QDir dir(outputDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create output directory:" << outputDir;
            emit processCompleted(false, "Failed to create output directory: " + outputDir);
            return QStringList();
        }
    }
    
    // Detect beats
    QVector<Beat> beats = detectBeats(inputFile, threshold);
    if (beats.isEmpty()) {
        qWarning() << "No beats detected in the file";
        emit processCompleted(false, "No beats detected in the file");
        return QStringList();
    }
    
    emit progressUpdate(0.3, "Detected beats, preparing for splitting...");
    
    // Create a Python script to split the file at beat positions
    QString scriptPath = m_tempDir + "/split_at_beats.py";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "import sys\n";
        out << "import os\n";
        out << "import json\n";
        out << "from moviepy.editor import VideoFileClip, AudioFileClip\n\n";
        out << "def split_at_beats(input_file, output_dir, beat_times):\n";
        out << "    # Determine if we're dealing with video or audio\n";
        out << "    base_name = os.path.basename(input_file)\n";
        out << "    name, ext = os.path.splitext(base_name)\n";
        out << "    is_video = ext.lower() in ['.mp4', '.mov', '.avi', '.mkv']\n\n";
        out << "    # Load the clip\n";
        out << "    try:\n";
        out << "        if is_video:\n";
        out << "            clip = VideoFileClip(input_file)\n";
        out << "        else:\n";
        out << "            clip = AudioFileClip(input_file)\n";
        out << "    except Exception as e:\n";
        out << "        print(f'Error loading file: {str(e)}')\n";
        out << "        return []\n\n";
        out << "    # Add file duration as the last beat time if it's not already there\n";
        out << "    if beat_times[-1] < clip.duration - 1.0:\n";
        out << "        beat_times.append(clip.duration)\n\n";
        out << "    # Add a 0.0 start time if the first beat is not at the beginning\n";
        out << "    if beat_times[0] > 0.5:\n";
        out << "        beat_times.insert(0, 0.0)\n\n";
        out << "    # Create output segments\n";
        out << "    output_files = []\n";
        out << "    total_segments = len(beat_times) - 1\n\n";
        out << "    for i in range(total_segments):\n";
        out << "        start_time = beat_times[i]\n";
        out << "        end_time = beat_times[i + 1]\n";
        out << "        \n";
        out << "        # Skip segments that are too short\n";
        out << "        if end_time - start_time < 0.1:\n";
        out << "            continue\n";
        out << "        \n";
        out << "        # Create a subclip\n";
        out << "        segment = clip.subclip(start_time, end_time)\n";
        out << "        \n";
        out << "        # Create output filename\n";
        out << "        segment_filename = f'{name}_segment_{i:03d}{ext}'\n";
        out << "        segment_path = os.path.join(output_dir, segment_filename)\n";
        out << "        \n";
        out << "        # Write the segment\n";
        out << "        try:\n";
        out << "            if is_video:\n";
        out << "                segment.write_videofile(segment_path, codec='libx264', audio_codec='aac')\n";
        out << "            else:\n";
        out << "                segment.write_audiofile(segment_path, codec='libmp3lame')\n";
        out << "            \n";
        out << "            output_files.append(segment_path)\n";
        out << "            print(f'PROGRESS:{(i + 1) / total_segments:.6f}')\n";
        out << "        except Exception as e:\n";
        out << "            print(f'Error writing segment {i}: {str(e)}')\n\n";
        out << "    # Close the clip\n";
        out << "    clip.close()\n";
        out << "    \n";
        out << "    return output_files\n\n";
        out << "if __name__ == '__main__':\n";
        out << "    if len(sys.argv) < 4:\n";
        out << "        print('Usage: python split_at_beats.py input_file output_dir beats_json')\n";
        out << "        sys.exit(1)\n\n";
        out << "    input_file = sys.argv[1]\n";
        out << "    output_dir = sys.argv[2]\n";
        out << "    beats_json = sys.argv[3]\n";
        out << "    \n";
        out << "    # Parse beat times from JSON\n";
        out << "    beat_times = json.loads(beats_json)\n";
        out << "    \n";
        out << "    output_files = split_at_beats(input_file, output_dir, beat_times)\n";
        out << "    \n";
        out << "    # Print output files as JSON\n";
        out << "    print('OUTPUT:' + json.dumps(output_files))\n";
        out << "    sys.exit(0)\n";
        scriptFile.close();
        
        // Prepare beat times as JSON
        QJsonArray beatArray;
        for (const Beat& beat : beats) {
            beatArray.append(beat.time);
        }
        QString beatsJson = QJsonDocument(beatArray).toJson(QJsonDocument::Compact);
        
        // Execute the Python script
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        
        QStringList args;
        args << scriptPath << inputFile << outputDir << beatsJson;
        
        process.start("python3", args);
        if (!process.waitForStarted()) {
            qWarning() << "Failed to start Python process for splitting";
            emit processCompleted(false, "Failed to start process for splitting");
            return QStringList();
        }
        
        // Monitor the process output for progress updates
        while (process.state() == QProcess::Running) {
            process.waitForReadyRead(100);
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split("\n", Qt::SkipEmptyParts);
            
            for (const QString& line : lines) {
                if (line.startsWith("PROGRESS:")) {
                    bool ok;
                    double progress = line.mid(9).toDouble(&ok);
                    if (ok) {
                        // Scale progress from 30% to 100%
                        emit progressUpdate(0.3 + progress * 0.7, "Splitting file...");
                    }
                }
            }
        }
        
        process.waitForFinished();
        
        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split("\n", Qt::SkipEmptyParts);
            
            for (const QString& line : lines) {
                if (line.startsWith("OUTPUT:")) {
                    QString jsonStr = line.mid(7);
                    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                    if (doc.isArray()) {
                        QJsonArray array = doc.array();
                        QStringList outputFiles;
                        for (const QJsonValue& value : array) {
                            outputFiles.append(value.toString());
                        }
                        
                        emit progressUpdate(1.0, "Splitting completed");
                        emit processCompleted(true, QString("Split into %1 segments").arg(outputFiles.size()));
                        return outputFiles;
                    }
                }
            }
            
            qWarning() << "Failed to parse output file list";
            emit processCompleted(false, "Failed to parse output file list");
            return QStringList();
        } else {
            QString errorOutput = process.readAllStandardOutput();
            qWarning() << "Failed to split file:" << errorOutput;
            emit processCompleted(false, "Failed to split file: " + errorOutput);
            return QStringList();
        }
    } else {
        qWarning() << "Failed to create Python script for splitting";
        emit processCompleted(false, "Failed to create Python script for splitting");
        return QStringList();
    }
}

QVector<BeatDetector::Beat> BeatDetector::detectBeatsWithPython(const QString& audioFile, double threshold)
{
    QVector<Beat> beats;
    
    // Check if Python module is available
    if (!m_aubioModule) {
        qWarning() << "Aubio Python module not initialized";
        return beats;
    }
    
    try {
        // Create a Python script to detect beats using Aubio
        QString scriptPath = m_tempDir + "/detect_beats.py";
        QFile scriptFile(scriptPath);
        if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&scriptFile);
            out << "import sys\n";
            out << "import json\n";
            out << "import aubio\n\n";
            out << "def detect_beats(input_file, threshold):\n";
            out << "    # Open the audio file\n";
            out << "    source = aubio.source(input_file)\n";
            out << "    samplerate = source.samplerate\n\n";
            out << "    # Create a beat detection object\n";
            out << "    tempo = aubio.tempo('default', 1024, 512, samplerate)\n";
            out << "    tempo.set_threshold(threshold)\n\n";
            out << "    # Storage for beat times and confidences\n";
            out << "    beats = []\n\n";
            out << "    # Process the audio\n";
            out << "    total_frames = source.duration\n";
            out << "    read_frames = 0\n";
            out << "    current_time = 0\n\n";
            out << "    while True:\n";
            out << "        samples, read = source()\n";
            out << "        is_beat = tempo(samples)\n";
            out << "        if is_beat:\n";
            out << "            confidence = tempo.get_confidence()\n";
            out << "            beats.append((current_time, confidence))\n\n";
            out << "        # Update time and position\n";
            out << "        current_time += read / samplerate\n";
            out << "        read_frames += read\n";
            out << "        if read < source.hop_size:\n";
            out << "            break\n\n";
            out << "        # Report progress\n";
            out << "        if total_frames > 0:\n";
            out << "            progress = read_frames / total_frames\n";
            out << "            sys.stdout.write(f'PROGRESS:{progress:.6f}\\n')\n";
            out << "            sys.stdout.flush()\n\n";
            out << "    return beats\n\n";
            out << "if __name__ == '__main__':\n";
            out << "    if len(sys.argv) < 3:\n";
            out << "        print('Usage: python detect_beats.py input_file threshold')\n";
            out << "        sys.exit(1)\n\n";
            out << "    input_file = sys.argv[1]\n";
            out << "    threshold = float(sys.argv[2])\n";
            out << "    \n";
            out << "    beats = detect_beats(input_file, threshold)\n";
            out << "    \n";
            out << "    # Print beats as JSON\n";
            out << "    print('BEATS:' + json.dumps(beats))\n";
            out << "    sys.exit(0)\n";
            scriptFile.close();
            
            // Execute the Python script
            QProcess process;
            process.setProcessChannelMode(QProcess::MergedChannels);
            
            QStringList args;
            args << scriptPath << audioFile << QString::number(threshold);
            
            process.start("python3", args);
            if (!process.waitForStarted()) {
                qWarning() << "Failed to start Python process for beat detection";
                return beats;
            }
            
            // Monitor the process output for progress updates
            while (process.state() == QProcess::Running) {
                process.waitForReadyRead(100);
                QString output = process.readAllStandardOutput();
                QStringList lines = output.split("\n", Qt::SkipEmptyParts);
                
                for (const QString& line : lines) {
                    if (line.startsWith("PROGRESS:")) {
                        bool ok;
                        double progress = line.mid(9).toDouble(&ok);
                        if (ok) {
                            emit progressUpdate(progress, "Analyzing audio...");
                        }
                    }
                }
            }
            
            process.waitForFinished();
            
            if (process.exitCode() == 0) {
                QString output = process.readAllStandardOutput();
                QStringList lines = output.split("\n", Qt::SkipEmptyParts);
                
                for (const QString& line : lines) {
                    if (line.startsWith("BEATS:")) {
                        QString jsonStr = line.mid(6);
                        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                        if (doc.isArray()) {
                            QJsonArray array = doc.array();
                            for (const QJsonValue& value : array) {
                                if (value.isArray()) {
                                    QJsonArray beatArray = value.toArray();
                                    if (beatArray.size() >= 2) {
                                        double time = beatArray.at(0).toDouble();
                                        double confidence = beatArray.at(1).toDouble();
                                        beats.append(Beat(time, confidence));
                                    }
                                }
                            }
                            return beats;
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in detectBeatsWithPython:" << e.what();
    }
    
    return beats;
}

QVector<BeatDetector::Beat> BeatDetector::detectBeatsWithCommandLine(const QString& audioFile, double threshold)
{
    QVector<Beat> beats;
    
    // Create a simple shell script to run aubio beat tracker
    QString scriptPath = m_tempDir + "/aubio_beat_tracker.sh";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "#!/bin/bash\n";
        out << "# Run aubio beat tracker on the input file\n";
        out << "aubio tempo -i \"" << audioFile << "\" -t " << threshold << " > \"" << m_tempDir << "/beats.txt\"\n";
        scriptFile.close();
        
        // Make the script executable
        QProcess::execute("chmod", QStringList() << "+x" << scriptPath);
        
        // Execute the script
        QProcess process;
        process.start(scriptPath);
        if (!process.waitForStarted()) {
            qWarning() << "Failed to start aubio process for beat detection";
            return beats;
        }
        
        emit progressUpdate(0.5, "Processing audio with aubio...");
        
        process.waitForFinished(-1);
        
        if (process.exitCode() == 0) {
            // Read the output file
            QFile outputFile(m_tempDir + "/beats.txt");
            if (outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&outputFile);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (!line.isEmpty()) {
                        bool ok;
                        double time = line.toDouble(&ok);
                        if (ok) {
                            beats.append(Beat(time, 1.0)); // Confidence not available from CLI
                        }
                    }
                }
                outputFile.close();
            }
        }
    }
    
    return beats;
}

void BeatDetector::cleanupTempFiles()
{
    // Clean up temporary files if needed
    QDir tempDir(m_tempDir);
    QStringList filters;
    filters << "*.py" << "*.sh" << "*.txt" << "*.wav" << "*.mp3";
    QStringList tempFiles = tempDir.entryList(filters, QDir::Files);
    
    for (const QString& file : tempFiles) {
        tempDir.remove(file);
    }
}
