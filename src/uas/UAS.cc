/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Represents one unmanned aerial vehicle
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QList>
#include <QMessageBox>
#include <QTimer>
#include <iostream>
#include <QDebug>
#include <cmath>
#include "UAS.h"
#include "LinkInterface.h"
#include "UASManager.h"
#include "MG.h"
#include "mavlink.h"

UAS::UAS(int id=0) :
        startTime(MG::TIME::getGroundTimeNow()),
        commStatus(COMM_DISCONNECTED),
        name(""),
        links(new QList<LinkInterface*>()),
        thrustSum(0),
        startVoltage(0),
        manualRollAngle(0),
        manualPitchAngle(0),
        manualYawAngle(0),
        manualThrust(0),
        controlRollManual(true),
        controlPitchManual(true),
        controlYawManual(true),
        currentVoltage(0.0f),
        controlThrustManual(true),
        lpVoltage(0.0f),
        mode(MAV_MODE_UNINIT),
        onboardTimeOffset(0)
{
    uasId = id;
    setBattery(LIPOLY, 3);
}

UAS::~UAS()
{
    delete links;
}

int UAS::getUASID()
{
    return uasId;
}

void UAS::setSelected()
{
    UASManager::instance()->setActiveUAS(this);
}

void UAS::receiveMessage(LinkInterface* link, mavlink_message_t message)
{


    if (!links->contains(link))
    {
        addLink(link);
    }

    if (message.sysid == uasId)
    {
        QString uasState;
        QString stateDescription;
        QString patternPath;
        bool uasIsAuto;
        switch (message.msgid)
        {
        case MAVLINK_MSG_ID_HEARTBEAT:
            emit heartbeat(this);
            // Set new type if it has changed
            if (this->type != message_heartbeat_get_type(&message))
            {
                this->type = message_heartbeat_get_type(&message);
                emit systemTypeSet(this, type);
            }
            break;
        case MAVLINK_MSG_ID_BOOT:
            //std::cerr << "Boot detected, software v." << std::dec << message_boot_get_version(message.payload) << " time: " << MG::TIME::getGroundTimeNow() << std::endl;
            //emit valueChanged(uasId, "Software version", message_boot_get_version(message.payload), MG::TIME::getGroundTimeNow());
            getStatusForCode((int)MAV_STATE_BOOT, uasState, stateDescription);
            emit statusChanged(this, uasState, stateDescription);
            onboardTimeOffset = 0; // Reset offset measurement
            break;
            /*
        case MAVLINK_MSG_ID_SYSTEM:
            //        std::cerr << std::endl;
            //        std::cerr << "System Message received" << std::endl;
            currentVoltage = message_system_get_voltage(message.payload)/1000.0f;
            filterVoltage(currentVoltage);
            if (startVoltage == 0) startVoltage = currentVoltage;
            timeRemaining = calculateTimeRemaining();
            //qDebug() << "Voltage: " << currentVoltage << " Chargelevel: " << getChargeLevel() << " Time remaining " << timeRemaining;
            emit batteryChanged(this, filterVoltage(), getChargeLevel(), timeRemaining);

            emit voltageChanged(message.acid, message_system_get_voltage(message.payload)/1000.0f);
            emit loadChanged(this, message_system_get_cpu_usage(message.payload)/100.0f);
            emit valueChanged(uasId, "Battery", message_system_get_voltage(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "CPU Load", message_system_get_cpu_usage(message.payload), MG::TIME::getGroundTimeNow());
            break;
        case MAVLINK_MSG_ID_ACTUATORS:
            emit actuatorChanged(this, 0, message_actuator_get_act1(message.payload));
            emit valueChanged(this->getUASID(), "Top Rotor", message_actuator_get_act1(message.payload), MG::TIME::getGroundTimeNow());
            emit actuatorChanged(this, 1, message_actuator_get_act2(message.payload));
            emit valueChanged(this->getUASID(), "Bottom Rotor", message_actuator_get_act2(message.payload), MG::TIME::getGroundTimeNow());
            emit actuatorChanged(this, 2, message_actuator_get_act3(message.payload));
            emit valueChanged(this->getUASID(), "Left Servo", message_actuator_get_act3(message.payload), MG::TIME::getGroundTimeNow());
            emit actuatorChanged(this, 3, message_actuator_get_act4(message.payload));
            emit valueChanged(this->getUASID(), "Right Servo", message_actuator_get_act4(message.payload), MG::TIME::getGroundTimeNow());
            // Calculate thrust sum
            thrustSum = (message_actuator_get_act1(message.payload) + message_actuator_get_act2(message.payload)) / 2;
            emit thrustChanged(this, thrustSum);
            break;
        case MAVLINK_MSG_ID_STATE:
            emit valueChanged(uasId, "State", message_state_get_state(message.payload), MG::TIME::getGroundTimeNow());
            getStatusForCode(message_state_get_state(message.payload), uasState, stateDescription);
            emit statusChanged(this, uasState, stateDescription);
            break;
        case MAVLINK_MSG_ID_POSITION:
            // Position message is displayed in meters and degrees
            {
                quint64 time = MG::TIME::getGroundTimeNow();//message_position_get_msec(message.payload);
                emit valueChanged(this, "x", message_position_get_posX(message.payload), time);
                emit valueChanged(this, "y", message_position_get_posY(message.payload), time);
                emit valueChanged(this, "z", message_position_get_posZ(message.payload), time);
                emit valueChanged(this, "roll", message_position_get_roll(message.payload), time);
                emit valueChanged(this, "pitch", message_position_get_pitch(message.payload), time);
                emit valueChanged(this, "yaw", message_position_get_yaw(message.payload), time);
                emit valueChanged(this, "x speed", message_position_get_speedX(message.payload), time);
                emit valueChanged(this, "y speed", message_position_get_speedY(message.payload), time);
                emit valueChanged(this, "z speed", message_position_get_speedZ(message.payload), time);
                emit valueChanged(this, "roll speed", message_position_get_speedRoll(message.payload), time);
                emit valueChanged(this, "pitch speed", message_position_get_speedPitch(message.payload), time);
                emit valueChanged(this, "yaw speed", message_position_get_speedYaw(message.payload), time);

                emit localPositionChanged(this, message_position_get_posX(message.payload), message_position_get_posY(message.payload), message_position_get_posZ(message.payload), time);
                emit valueChanged(uasId, "x", message_position_get_posX(message.payload), time);
                emit valueChanged(uasId, "y", message_position_get_posY(message.payload), time);
                emit valueChanged(uasId, "z", message_position_get_posZ(message.payload), time);
                emit valueChanged(uasId, "roll", message_position_get_roll(message.payload), time);
                emit valueChanged(uasId, "pitch", message_position_get_pitch(message.payload), time);
                emit valueChanged(uasId, "yaw", message_position_get_yaw(message.payload), time);
                emit valueChanged(uasId, "x speed", message_position_get_speedX(message.payload), time);
                emit valueChanged(uasId, "y speed", message_position_get_speedY(message.payload), time);
                emit valueChanged(uasId, "z speed", message_position_get_speedZ(message.payload), time);
                emit valueChanged(uasId, "roll speed", message_position_get_speedRoll(message.payload), time);
                emit valueChanged(uasId, "pitch speed", message_position_get_speedPitch(message.payload), time);
                emit valueChanged(uasId, "yaw speed", message_position_get_speedYaw(message.payload), time);
            }
            break;
        case MAVLINK_MSG_ID_MARKER:
            emit valueChanged(uasId, "marker confidence", message_marker_get_confidence(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "marker x", message_marker_get_posX(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "marker y", message_marker_get_posY(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "marker z", message_marker_get_posZ(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "marker roll", message_marker_get_roll(message.payload)/M_PI*180.0f, MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "marker pitch", message_marker_get_pitch(message.payload)/M_PI*180.0f, MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "marker yaw", message_marker_get_yaw(message.payload)/M_PI*180.0f, MG::TIME::getGroundTimeNow());
            break;
        case MAVLINK_MSG_ID_IMAGE:
            //std::cerr << std::endl;
            //std::cerr << "IMAGE DATA RECEIVED" << std::endl;
            emit imageStarted(message_image_get_rid(message.payload), message_image_get_width(message.payload), message_image_get_height(message.payload), message_image_get_depth(message.payload), message_image_get_channels(message.payload));
            break;
        case MAVLINK_MSG_ID_BYTES:
            //std::cerr << std::endl;
            //std::cerr << "BYTES RESSOURCE RECEIVED" << std::endl;
            emit imageDataReceived(message_bytestream_get_rid(message.payload), message_bytestream_get_data(message.payload), message_bytestream_get_length(message.payload), message_bytestream_get_index(message.payload));
            break;
        case MAVLINK_MSG_ID_SENSRAW:
            {
                quint64 time = message_sensraw_get_usec(message.payload);
                if (time == 0)
                {
                    time = MG::TIME::getGroundTimeNow();
                }
                else
                {
                    if (onboardTimeOffset == 0)
                    {
                        onboardTimeOffset = MG::TIME::getGroundTimeNow() - time;
                    }
                    time += onboardTimeOffset;
                }

                emit valueChanged(uasId, "Accel. X", message_sensraw_get_xacc(message.payload), time);
                emit valueChanged(uasId, "Accel. Y", message_sensraw_get_yacc(message.payload), time);
                emit valueChanged(uasId, "Accel. Z", message_sensraw_get_zacc(message.payload), time);
                emit valueChanged(uasId, "Gyro Phi", message_sensraw_get_xgyro(message.payload), time);
                emit valueChanged(uasId, "Gyro Theta", message_sensraw_get_ygyro(message.payload), time);
                emit valueChanged(uasId, "Gyro Psi", message_sensraw_get_zgyro(message.payload), time);
                emit valueChanged(uasId, "Mag. X", message_sensraw_get_xmag(message.payload), time);
                emit valueChanged(uasId, "Mag. Y", message_sensraw_get_ymag(message.payload), time);
                emit valueChanged(uasId, "Mag. Z", message_sensraw_get_zmag(message.payload), time);
                emit valueChanged(uasId, "Ground Dist.", message_sensraw_get_gdist(message.payload), time);
                emit valueChanged(uasId, "Pressure", message_sensraw_get_baro(message.payload), time);
                emit valueChanged(uasId, "Temperature", message_sensraw_get_temp(message.payload), time);
            }
            break;
            */
        case MAVLINK_MSG_ID_ATTITUDE:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << message_attitude_get_roll(message.payload) << " pitch: " << message_attitude_get_pitch(message.payload) << " yaw: " << message_attitude_get_yaw(message.payload) << std::endl;
            {
                quint64 time = message_attitude_get_msec(&message);
                if (time == 0)
                {
                    time = MG::TIME::getGroundTimeNow();
                }
                else
                {
                    if (onboardTimeOffset == 0)
                    {
                        onboardTimeOffset = MG::TIME::getGroundTimeNow() - time;
                    }
                    time += onboardTimeOffset;
                }
                emit valueChanged(uasId, "roll IMU", message_attitude_get_roll(&message), time);
                emit valueChanged(uasId, "pitch IMU", message_attitude_get_pitch(&message), time);
                emit valueChanged(uasId, "yaw IMU", message_attitude_get_yaw(&message), time);
                emit valueChanged(this, "roll IMU", message_attitude_get_roll(&message), time);
                emit valueChanged(this, "pitch IMU", message_attitude_get_pitch(&message), time);
                emit valueChanged(this, "yaw IMU", message_attitude_get_yaw(&message), time);
                emit attitudeChanged(this, message_attitude_get_roll(&message), message_attitude_get_pitch(&message), message_attitude_get_yaw(&message), time);
            }
            break;
            /*
        case MAVLINK_MSG_ID_DEBUG:
            emit valueChanged(uasId, QString("debug ") + QString::number(message_debug_get_index(message.payload)), message_debug_get_value(message.payload), MG::TIME::getGroundTimeNow());
            break;
        case MAVLINK_MSG_ID_MODE:
            {
                switch(static_cast<int>(message_mode_get_mode(message.payload)))
                {
                case (int)MAV_MODE_MANUAL:
                    {
                        uasIsAuto = false;
                    }
                    break;
                case (int)MAV_MODE_AUTO:
                    {
                        uasIsAuto = true;
                    }
                    break;
                default:
                    {
                        uasIsAuto = false;
                    }
                    break;
                }

                emit autoModeChanged(mode);
                emit valueChanged(uasId, "auto mode", static_cast<int>(message_mode_get_mode(message.payload)), MG::TIME::getGroundTimeNow());
                //qDebug() << "UAS MODE CHANGED TO AUTO, UAS MODE CHANGED TO AUTO, UAS MODE CHANGED TO AUTO, UAS MODE CHANGED TO AUTO, UAS MODE CHANGED TO AUTO, UAS MODE CHANGED TO AUTO, UAS MODE CHANGED TO AUTO:" << uasIsAuto;
            }
            break;
        case MAVLINK_MSG_ID_EMITWAYPT:
            emit waypointUpdated(this->getUASID(), message_emitwaypoint_get_id(message.payload), message_emitwaypoint_get_x(message.payload), message_emitwaypoint_get_y(message.payload), message_emitwaypoint_get_z(message.payload), message_emitwaypoint_get_yaw(message.payload), (message_emitwaypoint_get_autocontinue(message.payload) == 1 ? true : false), (message_emitwaypoint_get_active(message.payload) == 1 ? true : false));
            break;
        case MAVLINK_MSG_ID_GOTOWAYPT:
            emit valueChanged(uasId, "WP X", message_gotowaypoint_get_x(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "WP Y", message_gotowaypoint_get_y(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "WP Z", message_gotowaypoint_get_z(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "WP speed X", message_gotowaypoint_get_speedX(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "WP speed Y", message_gotowaypoint_get_speedY(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "WP speed Z", message_gotowaypoint_get_speedZ(message.payload), MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, "WP yaw", message_gotowaypoint_get_yaw(message.payload)/M_PI*180.0f, MG::TIME::getGroundTimeNow());
            break;
        case MAVLINK_MSG_ID_WP_REACHED:
            qDebug() << "WAYPOINT REACHED";
            emit waypointReached(this, message_wp_reached_get_id(message.payload));
            break;
        case MAVLINK_MSG_ID_DETECTION:
            patternPath = QString(message_detection_get_patternpath(message.payload));
            //qDebug() << "OBJECT DETECTED";
            emit detectionReceived(uasId, patternPath, 0, 0, 0, 0, 0, 0, 0, 0, message_detection_get_confidence(message.payload), (message_detection_get_detected(message.payload) == 1 ? true : false ));
            break;
        default:
            std::cerr << "Unable to decode message from system " << std::dec << static_cast<int>(message.acid) << " with message id:" << static_cast<int>(message.msgid) << std::endl;
            //qDebug() << std::cerr << "Unable to decode message from system " << std::dec << static_cast<int>(message.acid) << " with message id:" << static_cast<int>(message.msgid) << std::endl;
            break;
        */
        }
    }

    //    virtual void attitudeChanged(int uasId, double roll, double pitch, double yaw, quint64 usec);
    //    virtual void localPositionChanged(int uasId, double x, double y, double z, quint64 usec);
    //    virtual void globalPositionChanged(int uasId, double lon, double lat, double alt, quint64 usec);
    // virtual void actuatorsChanged(int uasId, double actuator1, double actuator2, double actuator3, double actuator4, double actuator5);
    // virtual void voltageChanged(int uasId, double voltage);
}

void UAS::setAutoMode(bool autoMode)
{
    mavlink_message_t msg;
    if (autoMode)
    {
        message_set_mode_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, getUASID(), (unsigned char)MAV_MODE_AUTO);
        mode = MAV_MODE_AUTO;
        sendMessage(msg);
    }
    else
    {
        message_set_mode_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, getUASID(), (unsigned char)MAV_MODE_MANUAL);
        mode = MAV_MODE_MANUAL;
        sendMessage(msg);
    }
}

void UAS::setMode(enum MAV_MODE mode)
{
    this->mode = mode;
    mavlink_message_t msg;
    message_set_mode_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, getUASID(), (unsigned char)mode);
    sendMessage(msg);
}

void UAS::sendMessage(mavlink_message_t message)
{
    qDebug() << "In send message";
    // Emit message on all links that are currently connected
    QList<LinkInterface*>::iterator i;
    qDebug() << "LINKS: " << links->length();
    for (i = links->begin(); i != links->end(); ++i)
    {
        //        if (i != NULL)
        //        {
        qDebug() << "UAS::sendMessage()";
        sendMessage(*i, message);

    }
}

void UAS::sendMessage(LinkInterface* link, mavlink_message_t message)
{
    qDebug() << "UAS::sendMessage(link, message)";
    // Create buffer
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    // Write message into buffer, prepending start sign
    int len = message_to_send_buffer(buffer, &message);
    // If link is connected
    if (link->isConnected())
    {
        // Send the portion of the buffer now occupied by the message
        link->writeBytes((const char*)buffer, len);
    }
}

float UAS::filterVoltage()
{
    return lpVoltage;
}

/**
 * @param value battery voltage
 */
float UAS::filterVoltage(float value)
{
    /*
    currentVoltage = value;
    static QList<float> voltages<float>(20);
    const int dropPercent = 20;
    voltages.takeFirst();
    voltages.append(value);
    // Drop top and bottom xx percent of values
    QList<float> v(voltages);
    qSort(v);
    v = QList::mid(v.size()/dropPercent, v.size() - v.size()/dropPercent);
    lpVoltage = 0.0f;
    foreach (float value, v)
    {
        lpVoltage += v;
    }*/
    return currentVoltage;
}

void UAS::getStatusForCode(int statusCode, QString& uasState, QString& stateDescription)
{
    switch (statusCode)
    {
            case MAV_STATE_UNINIT:
        uasState = tr("UNINIT");
        stateDescription = tr("Not initialized");
        break;
                                case MAV_STATE_BOOT:
        uasState = tr("BOOT");
        stateDescription = tr("Booting system, please wait..");
        break;
                                case MAV_STATE_CALIBRATING:
        uasState = tr("CALIBRATING");
        stateDescription = tr("Calibrating sensors..");
        break;
                                case MAV_STATE_ACTIVE:
        uasState = tr("ACTIVE");
        stateDescription = tr("Normal operation mode");
        break;
                                case MAV_STATE_CRITICAL:
        uasState = tr("CRITICAL");
        stateDescription = tr("Failure occured!");
        break;
                                case MAV_STATE_EMERGENCY:
        uasState = tr("EMERGENCY");
        stateDescription = tr("EMERGENCY: Please land");
        break;
                                case MAV_STATE_POWEROFF:
        uasState = tr("SHUTDOWN");
        stateDescription = tr("Powering off system");
        break;
                                default:
        uasState = tr("UNKNOWN");
        stateDescription = tr("FAILURE: Unknown system state");
        break;
    }
}



/* MANAGEMENT */

/*
 *
 * @return The uptime in milliseconds
 *
 **/
quint64 UAS::getUptime()
{
    if(startTime == 0) {
        return 0;
    } else {
        return MG::TIME::getGroundTimeNow() - startTime;
    }
}

int UAS::getCommunicationStatus()
{
    return commStatus;
}

void UAS::requestWaypoints()
{
    mavlink_message_t message;
    //messagePackGetWaypoints(MG::SYSTEM::ID, &message); FIXME
    sendMessage(message);
    qDebug() << "UAS Request WPs";
}

/**
 * @brief Launches the system
 *
 **/
void UAS::launch()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(),(int)MAV_ACTION_LAUNCH);
    sendMessage(message);
    qDebug() << "UAS LAUNCHED!";
    //emit commandSent(LAUNCH);
}

/**
 * Depending on the UAS, this might make the rotors of a helicopter spinning
 *
 **/
void UAS::enable_motors()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(),(int)MAV_ACTION_MOTORS_START);
    sendMessage(message);
}

/**
 * @warning Depending on the UAS, this might completely stop all motors.
 *
 **/
void UAS::disable_motors()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(),(int)MAV_ACTION_MOTORS_STOP);
    sendMessage(message);
}

void UAS::setManualControlCommands(double roll, double pitch, double yaw, double thrust)
{
    // Scale values
    double rollPitchScaling = 0.2f;
    double yawScaling = 0.5f;
    double thrustScaling = 1.0f;

    manualRollAngle = roll * rollPitchScaling;
    manualPitchAngle = pitch * rollPitchScaling;
    manualYawAngle = yaw * yawScaling;
    manualThrust = thrust * thrustScaling;

    if(mode == MAV_MODE_MANUAL)
    {
        mavlink_message_t message;
        message_manual_control_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->uasId, (float)manualRollAngle, (float)manualPitchAngle, (float)manualYawAngle, (float)manualThrust, controlRollManual, controlPitchManual, controlYawManual, controlThrustManual);
        sendMessage(message);
        qDebug() << __FILE__ << __LINE__ << ": SENT MANUAL CONTROL MESSAGE: roll" << manualRollAngle << " pitch: " << manualPitchAngle << " yaw: " << manualYawAngle << " thrust: " << manualThrust;

        emit attitudeThrustSetPointChanged(this, roll, pitch, yaw, thrust, MG::TIME::getGroundTimeNow());
    }
}

void UAS::receiveButton(int buttonIndex)
{
    switch (buttonIndex)
    {
    case 0:
        setAutoMode(false);
        break;
    case 1:
        setAutoMode(true);
        break;
    default:

        break;
    }
    qDebug() << __FILE__ << __LINE__ << ": Received button clicked signal (button # is: " << buttonIndex << "), UNIMPLEMENTED IN MAVLINK!";

}

void UAS::setWaypoint(Waypoint* wp)
{
    mavlink_message_t message;
    // FIXME
    //messagePackSetWaypoint(MG::SYSTEM::ID, &message, wp->id, wp->x, wp->y, wp->z, wp->yaw, (wp->autocontinue ? 1 : 0));
    sendMessage(message);
    qDebug() << "UAS SENT Waypoint " << wp->id;
}

void UAS::setWaypointActive(int id)
{
    mavlink_message_t message;
    // FIXME
    //messagePackChooseWaypoint(MG::SYSTEM::ID, &message, id);
    sendMessage(message);
    // TODO This should be not directly emitted, but rather being fed back from the UAS
    emit waypointSelected(getUASID(), id);
}


void UAS::halt()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(), (int)MAV_ACTION_HALT);
    sendMessage(message);
}

void UAS::go()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(), (int)MAV_ACTION_CONTINUE);
    sendMessage(message);
}

/** Order the robot to return home / to land on the runway **/
void UAS::home()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(), (int)MAV_ACTION_RETURN);
    sendMessage(message);
}

/**
 * The MAV starts the emergency landing procedure. The behaviour depends on the onboard implementation
 * and might differ between systems.
 */
void UAS::emergencySTOP()
{
    mavlink_message_t message;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(), (int)MAV_ACTION_EMCY_LAND);
}

/**
 * All systems are immediately shut down (e.g. the main power line is cut).
 * @warning This might lead to a crash
 *
 * The command will not be executed until emergencyKILLConfirm is issues immediately afterwards
 */
bool UAS::emergencyKILL()
{
    bool result = false;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("EMERGENCY: KILL ALL MOTORS ON UAS");
    msgBox.setInformativeText("Do you want to cut power on all systems?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    // Close the message box shortly after the click to prevent accidental clicks
    QTimer::singleShot(5000, &msgBox, SLOT(reject()));


    if (ret == QMessageBox::Yes)
    {
        mavlink_message_t message;
        // TODO Replace MG System ID with static function call and allow to change ID in GUI
        message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(), (int)MAV_ACTION_EMCY_KILL);
        sendMessage(message);
        result = true;
    }
    return result;
}

void UAS::shutdown()
{
    bool result = false;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("Shutting down the UAS");
    msgBox.setInformativeText("Do you want to shut down the onboard computer?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    // Close the message box shortly after the click to prevent accidental clicks
    QTimer::singleShot(5000, &msgBox, SLOT(reject()));


    if (ret == QMessageBox::Yes)
    {
        // If the active UAS is set, execute command
        mavlink_message_t message;
        // TODO Replace MG System ID with static function call and allow to change ID in GUI
        message_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->getUASID(), (int)MAV_ACTION_SHUTDOWN);
        sendMessage(message);
        result = true;
    }
}

/**
 * @return The name of this system as string in human-readable form
 */
QString UAS::getUASName()
{
    QString result;
    if (name == "")
    {
        result = tr("MAV ") + result.sprintf("%03d", getUASID());
    }
    else
    {
        result = name;
    }
    return result;
}

void UAS::addLink(LinkInterface* link)
{
    if (!links->contains(link))
    {
        links->append(link);
    }
    //links->append(link);
    //qDebug() << " ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK";
}

/**
 * @brief Get the links associated with this robot
 *
 **/
QList<LinkInterface*>* UAS::getLinks()
{
    return links;
}



void UAS::setBattery(BatteryType type, int cells)
{
    this->batteryType = type;
    this->cells = cells;
    switch (batteryType)
    {
            case NICD:
        break;
            case NIMH:
        break;
            case LIION:
        break;
            case LIPOLY:
        fullVoltage = this->cells * UAS::lipoFull;
        emptyVoltage = this->cells * UAS::lipoEmpty;
        break;
            case LIFE:
        break;
            case AGZN:
        break;
    }
}

int UAS::calculateTimeRemaining()
{
    quint64 dt = MG::TIME::getGroundTimeNow() - startTime;
    double seconds = dt / 1000.0f;
    double voltDifference = startVoltage - currentVoltage;
    if (voltDifference <= 0) voltDifference = 0.00000000001f;
    double dischargePerSecond = voltDifference / seconds;
    int remaining = static_cast<int>((currentVoltage - emptyVoltage) / dischargePerSecond);
    // Can never be below 0
    if (remaining < 0) remaining = 0;
    return remaining;
}

double UAS::getChargeLevel()
{
    return 100.0f * ((lpVoltage - emptyVoltage)/(fullVoltage - emptyVoltage));
}

void UAS::clearWaypointList()
{
    mavlink_message_t message;
    // FIXME
    //messagePackRemoveWaypoints(MG::SYSTEM::ID, &message);
    sendMessage(message);
    qDebug() << "UAS clears Waypoints!";
}