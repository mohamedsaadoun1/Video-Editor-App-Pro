/**
 * @file beat_detector.h
 * @brief Header for beat detection services
 */

#ifndef BEAT_DETECTOR_H
#define BEAT_DETECTOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPair>
#include <QProcess>
#include <Python.h>

/**
 * @class BeatDetector
 * @brief Provides beat detection capabilities for music synchronization
 * 
 * This class uses Aubio for beat detection in audio files and provides
 * methods to find beats, align clips to beats, and visualize beats
 * on the timeline.
 */
class BeatDetector : public QObject
{
    Q_OBJECT

public:
    explicit BeatDetector(QObject *parent = nullptr);
    ~BeatDetector();

    /**
     * @brief Initialize Python interpreter and Aubio module
     * @return True if successful, false otherwise
     */
    bool initializePython();

    /**
     * @struct Beat
     * @brief Represents a detected beat with time and confidence
     */
    struct Beat {
        double time;       // Time in seconds
        double confidence; // Confidence level (0.0-1.0)
        Beat(double t, double c) : time(t), confidence(c) {}
    };

    /**
     * @brief Detect beats in an audio file
     * @param audioFile Path to the audio file
     * @param threshold Confidence threshold (0.0-1.0)
     * @return List of detected beats with their times and confidences
     */
    Q_INVOKABLE QVector<Beat> detectBeats(const QString& audioFile, double threshold = 0.3);

    /**
     * @brief Get beat times as a QVariantList for QML
     * @param audioFile Path to the audio file
     * @param threshold Confidence threshold (0.0-1.0)
     * @return List of beat times in seconds
     */
    Q_INVOKABLE QVariantList getBeatTimes(const QString& audioFile, double threshold = 0.3);

    /**
     * @brief Detect tempo of an audio file
     * @param audioFile Path to the audio file
     * @return Detected tempo in BPM (beats per minute)
     */
    Q_INVOKABLE double detectTempo(const QString& audioFile);

    /**
     * @brief Split an audio or video at detected beat positions
     * @param inputFile Path to the input file
     * @param outputDir Directory to save the split segments
     * @param threshold Confidence threshold for beat detection
     * @return List of output file paths
     */
    Q_INVOKABLE QStringList splitAtBeats(const QString& inputFile, const QString& outputDir, double threshold = 0.3);

signals:
    /**
     * @brief Signal emitted during a process to indicate progress
     * @param progress Progress value from 0.0 to 1.0
     * @param message Optional status message
     */
    void progressUpdate(double progress, const QString& message = "");
    
    /**
     * @brief Signal emitted when a process completes
     * @param success Whether the process completed successfully
     * @param message Success or error message
     */
    void processCompleted(bool success, const QString& message);

    /**
     * @brief Signal emitted when beat detection is complete
     * @param beats List of detected beat times
     */
    void beatsDetected(const QVariantList& beats);

private:
    PyObject* m_aubioModule;
    QString m_tempDir;
    QProcess m_process;

    // Use Python-based Aubio for beat detection
    QVector<Beat> detectBeatsWithPython(const QString& audioFile, double threshold);
    
    // Fallback to using Aubio via command line if Python module fails
    QVector<Beat> detectBeatsWithCommandLine(const QString& audioFile, double threshold);
    
    // Helper functions
    void createTempDir();
    void cleanupTempFiles();
};

#endif // BEAT_DETECTOR_H
