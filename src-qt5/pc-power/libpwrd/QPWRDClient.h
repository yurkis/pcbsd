#ifndef LIBPWRD_H
#define LIBPWRD_H

#include <QString>
#include <QVector>
#include <QObject>

#include "pwrdtypes.h"

//! Structure holds supported hardware info
typedef struct PWRDHardwareInfo
{
    PWRHWInfo basic;                        ///< Basic hardware info
    QVector<PWRBatteryHardware>  batteries; ///< Batteries hardware info
    QVector<PWRBacklightHardware>backlights;///< Backlights hardware info
}PWRDHardwareInfo;

//! Basic profile info. For full profile data: \see PWRProfile
typedef struct PWRProfileInfoBasic
{
    QString id;     ///< Profile ID
    QString name;   ///< Profile description
}PWRProfileInfoBasic;

//! Private class
class QPWRDClientPrivate;

//! PWRD daemon client class
class QPWRDClient:public QObject
{    
    Q_OBJECT        
    Q_DECLARE_PRIVATE(QPWRDClient)
public:
    explicit QPWRDClient(QObject *parent=0);
    ~QPWRDClient();

    //! Connect to pwrd server
    /*!
     * \param pipe- full path to pwrd control pipe
     *
     * \return true if success
     */
    virtual bool connect(QString pipe = QString(DEF_PWRD_PIPE_NAME));

    //! Disconnect from pwrd
    virtual void disconnect();

    //! Get last pwrd error
    /*!
     * \return pwrd error text. If no error- empty string
     */
    virtual QString lastPWRDError();

    //! Get hardware info
    /*!
     * \param[out] out - hardware info
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getHardwareInfo(PWRDHardwareInfo &out);

    //! Get levels of all supported backlights
    /*!
     *
     * \param[out] out - vector of backlight levels. Index in vaector is backlight number
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getAllBacklighsLevel(QVector<int>& out);

    //!Get specified backlight level
    /*!
     * \param[in] backlight - backlight number
     * \param[out] out - hardware info
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getBacklightLevel(int backlight, int& out);

    //! Set backlight level
    /*!
     * \param level - backlight level value in percent (0..100)
     * \param backlight - backlight nuber or PWR_ALL for all backlights
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setBacklightLevel(int level, int backlight = PWR_ALL);

    //! Set backlight level relative value
    /*!
     * \param level - relative backlight level value in percent. Negative numbers are allowed
     * \param backlight - backlight nuber or PWR_ALL for all backlights
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setBacklightLevelRelative(int level, int backlight = PWR_ALL);

    //! Set backlight level as string
    /*!
     * \param level - backlight level value in percent. Strinnng value. Use + and  - for relative values (for example "+15" or "-10")
     * \param backlight - backlight nuber or PWR_ALL for all backlights
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setBacklightLevel(QString level, int backlight = PWR_ALL);

    //! Get current profiles
    /*!
     * \param[out] ac_profile - profile on ac power (NULL if not need)
     * \param[out] batt_profile - profile on battery power  (NULL if not need)
     * \param[out] low_batt_profile - profile on low battery level (NULL if not need)
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getActiveProfiles(PWRProfileInfoBasic* ac_profile, PWRProfileInfoBasic* batt_profile, PWRProfileInfoBasic* low_batt_profile);

    //! Get all profiles
    /*!
     * param[out] profiles - vector of profiles
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getProfiles(QVector<PWRProfileInfoBasic>& profiles);

    //! Get profile
    /*!
     * \param[in] profile_id - profile id
     * \param[out] out - profile
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getProfile(QString profile_id, PWRProfile& out);

    //!  Get id of current profile
    /*!
     * \param[out] out - current profile id
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getCurrentProfileID(PWRProfileInfoBasic& out);

    //!  Change current profile
    /*!
     * \param profile_id - profile id
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setCurrentProfile(QString profile_id);

    //! Get current AC line state
    /*!
     * \param[out] isOnACPower - true if device is on external power
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getACLineState(bool &isOnACPower);

    //! Get state of all supported batteries
    /*!
     * \param[out] batteries - vector with batteries status. Index in vector is battery number
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getBatteriesState(QVector<PWRBatteryStatus> &batteries);

    //! Set system ACPI state (sleep, hibernate)
    /*!
     * \param state - state to set ("S3" for sleep, "S4" for hibernate)
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setACPIState(QString state);

    //! Get current ACPI buttons setings
    /*!
     * \param[out] powerBtnSate - power button ACPI state
     * \param[out] sleepBtnState - sleep button ACPI state
     * \param[out] lidState - lid switch state
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getButtonsState(QString& powerBtnSate, QString& sleepBtnState, QString& lidState);

    //! Set current ACPI buttons setings
    /*!
     * \param powerBtnSate - power button ACPI state
     * \param sleepBtnState - sleep button ACPI state
     * \param lidState - lid switch state
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setButtonsState(QString* powerBtnSate, QString* sleepBtnState, QString* lidState);

    //! Get daemon settings
    /*!
     * \param[out] settings - current daemon settings
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool getDaemonSettings(PWRDaemonSettings& settings);

    //!  Set daemon settings
    /*!
     * Set daemon settings. Not all settings will be applied and saved. Operation should be allowed in daemon settings file
     *
     * \param[out] settings - current daemon settings
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool setDaemonSettings(PWRDaemonSettings settings);

    //! Update or create profile
    /*!
     * Create new profile or change profile if exist
     *
     * \param profile - new rofile
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool updateProfile(PWRProfile profile);

    //!  Remove profile
    /*!
     * Remove profile. Operation should be allowed in daemon settings file
     *
     * \param profile_id - id of profile to remove
     *
     * \return true if success. If false check lastPWRDError()
     */
    virtual bool removeProfile(QString profile_id);

signals:

    //! Connection error signal
    /*!
     * Signal will be emmitrd when client could not connected to PWRD or connection was lost
     */
    void connectionError();

    //! PWRD error signal
    /*!
     * Signal will be emmited when PWRD returns error
     *
     * \param message - pwrd error message string
     */
    void pwrdError(QString message);

private slots:

protected:
     QPWRDClientPrivate * const d_ptr;
};



#endif // LIBPWRD_H
