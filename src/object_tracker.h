/**
 * @file object_tracker.h
 * @brief Header for object tracking functionality
 */

#ifndef OBJECT_TRACKER_H
#define OBJECT_TRACKER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QRect>
#include <QVector>
#include <QProcess>

// Forward declaration for OpenCV types
namespace cv {
    class Mat;
    class Tracker;
}

/**
 * @class ObjectTracker
 * @brief Provides object tracking capabilities for videos
 * 
 * This class uses OpenCV's tracking algorithms to track objects in videos
 * and allows applying effects or text to follow the tracked objects.
 */
class ObjectTracker : public QObject
{
    Q_OBJECT

public:
    explicit ObjectTracker(QObject *parent = nullptr);
    ~ObjectTracker();

    /**
     * @brief Enumerate available tracking algorithms
     * @return List of tracker names and descriptions
     */
    Q_INVOKABLE QVariantList availableTrackers() const;

    /**
     * @enum TrackingAlgorithm
     * @brief Supported tracking algorithms
     */
    enum class TrackingAlgorithm {
        KCF,      // Kernelized Correlation Filters
        CSRT,     // Discriminative Correlation Filter with Channel and Spatial Reliability
        MOSSE,    // Minimum Output Sum of Squared Error
        BOOSTING, // Boosting
        MIL,      // Multiple Instance Learning
        TLD,      // Tracking, Learning and Detection
        MEDIANFLOW // Median Flow
    };
    Q_ENUM(TrackingAlgorithm)

    /**
     * @struct TrackingResult
     * @brief Represents tracking results for a frame
     */
    struct TrackingResult {
        int frameNumber;       // Frame number
        QRect boundingBox;     // Bounding box (x, y, width, height)
        double confidence;     // Tracking confidence (0.0-1.0)
        
        TrackingResult(int frame, const QRect& bbox, double conf = 1.0)
            : frameNumber(frame), boundingBox(bbox), confidence(conf) {}
    };

    /**
     * @brief Track an object in a video
     * @param videoFile Path to the video file
     * @param initialRect Initial bounding box (x, y, width, height)
     * @param algorithm Tracking algorithm to use
     * @param startFrame Starting frame (default: 0)
     * @param endFrame Ending frame (default: -1 for end of video)
     * @return Success or failure
     */
    Q_INVOKABLE bool trackObject(
        const QString& videoFile,
        const QRect& initialRect,
        TrackingAlgorithm algorithm = TrackingAlgorithm::KCF,
        int startFrame = 0,
        int endFrame = -1
    );

    /**
     * @brief Get tracking results
     * @return Vector of tracking results
     */
    Q_INVOKABLE QVariantList getTrackingResults() const;

    /**
     * @brief Create a video with visualized tracking results
     * @param inputFile Input video file
     * @param outputFile Output video file
     * @param showBoundingBox Whether to show the bounding box
     * @param showTrajectory Whether to show the object trajectory
     * @param labelText Text to display near the tracked object
     * @return Success or failure
     */
    Q_INVOKABLE bool createTrackedVideo(
        const QString& inputFile,
        const QString& outputFile,
        bool showBoundingBox = true,
        bool showTrajectory = true,
        const QString& labelText = ""
    );

    /**
     * @brief Apply an effect to the tracked object in a video
     * @param inputFile Input video file
     * @param outputFile Output video file
     * @param effectType Effect type (blur, pixelate, highlight, overlay)
     * @param effectParams Additional parameters for the effect
     * @return Success or failure
     */
    Q_INVOKABLE bool applyEffectToTrackedObject(
        const QString& inputFile,
        const QString& outputFile,
        const QString& effectType,
        const QVariantMap& effectParams
    );

    /**
     * @brief Extract the tracked object as a separate video with alpha channel
     * @param inputFile Input video file
     * @param outputFile Output video file
     * @param expandRect Expand the bounding box by this factor (e.g., 1.1 = 10% larger)
     * @return Success or failure
     */
    Q_INVOKABLE bool extractTrackedObject(
        const QString& inputFile,
        const QString& outputFile,
        double expandRect = 1.0
    );

    /**
     * @brief Calculate object motion data for use with keyframes
     * @return Motion data as a JSON string
     */
    Q_INVOKABLE QString calculateMotionKeyframes() const;

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
     * @param outputPath Path to the output file if applicable
     */
    void processCompleted(bool success, const QString& message, const QString& outputPath = "");

    /**
     * @brief Signal emitted when tracking is complete
     * @param results Tracking results
     */
    void trackingCompleted(const QVariantList& results);

private:
    QString m_videoFile;
    QVector<TrackingResult> m_trackingResults;
    QProcess m_process;
    QString m_tempDir;

    // Helper methods
    cv::Tracker* createTracker(TrackingAlgorithm algorithm);
    void processVideo(
        const QString& inputFile,
        const QString& outputFile,
        const std::function<void(cv::Mat&, int, const QRect&)>& frameProcessor
    );
    QVariantMap trackingResultToVariantMap(const TrackingResult& result) const;
    void createTempDir();
    void cleanupTempFiles();
};

#endif // OBJECT_TRACKER_H
