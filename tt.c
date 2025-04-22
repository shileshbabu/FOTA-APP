

MQTTAsync deviceClient;
MQTTAsync gatewayClient;
void RouteGatewayMQTT(char* spayload, char* stopic);
int idevice_agent_disc_finished = 0;
int idevice_agent_Subscribed = 0;
int idevice_agent_Finished = 0;

int igateway_agent_disc_finished = 0;
int igateway_agent_Subscribed = 0;
int igateway_agent_Finished = 0;

char sGatewayInitializationPayload[MAXPAYLOADSIZE];
char sGatewayPublishTopic[SIZE_512];
char sGatewaySubscribeTopic[SIZE_512];

void (*response_GatewayMQTTptr)(char*, char*);
void (*response_DeviceMQTTptr)(char*, char*);
void(*status_ptrmqtt)(MQTTConnentState);
char* pszTopic;
char* pszPayload;
bool bFlagGatewayPub;
int offlineMode;

#ifdef _WIN32
extern HANDLE g_hRestartSuspendEvent;
#else
extern pthread_cond_t  g_SuspendAgentCond;
#endif

void PublishRespToDevice(char *pszTtl, const char* pszPublicTopic, char* pszMessage) {

    LOG_INFO("Audit log START --> PublishRespToDevice");

    if ((pszTtl != NULL) && (strlen(pszTtl) > 0))
    {
        LOG_INFO("TTL value from Gateway Response = [%s]", pszTtl);
        if (hasTtlExpired(pszTtl))
        {
            LOG_ERROR("TTL expired for response of device request");
            return;
        }
    }

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    LOG_INFO("Input Message: %s Message Length: %d--> PublishRespToDevice", pszMessage, strlen(pszMessage));

    pubmsg.payload = (void *)pszMessage;
    pubmsg.payloadlen = strlen((char *)pubmsg.payload);
    pubmsg.qos = 2;
    pubmsg.retained = 0;

    if(!strcmp(pszPublicTopic, MQTT_LWT_EVENT_TOPIC)) {
        /* If its LWT topic, set retained to true for clearing the old LWT message */
        pubmsg.retained = 1;
    }

    LOG_INFO("pubmsg: %s pubmsg length: %d --> PublishRespToDevice", (char *)(pubmsg.payload), pubmsg.payloadlen);
    
    int rc = 0;
    if ((rc = MQTTAsync_sendMessage(deviceClient, pszPublicTopic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to sendMessage, return code %d   --> PublishRespToDevice", rc);
    }

    LOG_INFO("Audit log END --> PublishRespToDevice");
}

#if 0

/*
*
* Function:      publishToDevice
* Description :  Function to publish devices
*                Mqtt topic
* Return value:  void
* Argument:      char*(status), char*(message)
* Note:          MQTT specific  user utility
*
*/
void publishToDevice(char* sStatus, char* sMessage) {

    LOG_INFO("Audit log START --> publishToDevice");

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    char sPayload[MAXPAYLOADSIZE];
    sprintf(sPayload,
        "{ \"gatewayServer\": \"%s\", \"gatewayPort\": \"%d\" \"deviceId\": \"%s\", \"deviceName\": \"%s\", \"status\": \"%s\", \"message\": \"%s\" }",
        GetGetwayServerMqtt(GATEWAY_SERVER), g_IgatewayPort, g_SdeviceID, g_szdeviceName, sStatus, sMessage);
    pubmsg.payload = sPayload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 2;
    pubmsg.retained = 0;
	#ifdef _WIN32
		Sleep(100);
	#else
		usleep(1000);
	#endif
    int rc = 0;
    char* sPubToDeviceTopic = malloc(128);
    LOG_INFO("Sending message to device: %s --> publishToDevice", sPayload);
    if ((rc = MQTTAsync_sendMessage(deviceClient, PUBLISH_RSP_TO_DEVICE_PRF_TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to sendMessage, return code %d   --> publishToDevice", rc);
    }

    LOG_INFO("Audit log END --> publishToDevice");
}


#endif
/*
*
* Function:      publishCommandToDevice
* Description :  Function publish message to device
*
* Return value:  Void
* Argument:      char *(topic), char * (payload)
* Note:          MQTT specific  user utility
*
*/
void publishCommandToDevice(char *pszTtl, char* sCommandToDeviceTopic, char* sCommandToDevice) 
{
    LOG_INFO("Audit log START --> publishCommandToDevice");

    if ((pszTtl != NULL) && (strlen(pszTtl) > 0))
    {
        LOG_INFO("TTL value from Gateway Response = [%s]", pszTtl);
        if (hasTtlExpired(pszTtl))
        {
            LOG_ERROR("TTL expired for response of device request");
            return;
        }
    }

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    char* sPayload = (char*)malloc(MAXPAYLOADSIZE * sizeof(char));

    strcpy(sPayload, sCommandToDevice);
    pubmsg.payload = sPayload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 2;
    pubmsg.retained = 0;

    int rc = 0;
    LOG_INFO("Sending message to device: Topic - %s message - %s --> publishCommandToDevice", sCommandToDeviceTopic, sCommandToDevice);


    if ((rc = MQTTAsync_sendMessage(deviceClient, sCommandToDeviceTopic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to sendMessage, return code %d   --> publishCommandToDevice", rc);
    }

    LOG_INFO("Sent message to device\n");
    free(sPayload);

    LOG_INFO("Audit log END --> publishCommandToDevice");
}



/*
*
* Function:      on_device_agent_mqtt_message
* Description :  Function publish message from device to agent
*
* Return value:  Void
* Argument:     void* context(MQTT context), char * (topic Name), int (topic size), MQTTAsync_message *( MQTT message)
* Note:         MQTT  protocol specific  callback  utility
*
*/
int on_device_agent_mqtt_message(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    (void) (context);
    (void) (topicLen);
    LOG_INFO("Audit log START --> on_device_agent_mqtt_message");
    LOG_INFO("Received message from device %s \n", (char *)(message->payload));

    const char *pszHeader = getJsonObject((char *)(message->payload), "header");
    if (pszHeader == NULL)
    {
        LOG_ERROR("Did not receive header information from device. Not processing the message.");
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free((void *)topicName);
        return 1;
    }

    char *pszTtl = getJsonData(pszHeader, AGENT_TTL_STRING);
    if ((pszTtl != NULL) && (strlen(pszTtl) > 0))
    {
        LOG_INFO("TTL = [%s]", pszTtl);
        if (hasTtlExpired(pszTtl))
        {
            LOG_ERROR("TTL expired for topic [%s]", topicName);
            MQTTAsync_freeMessage(&message);
            MQTTAsync_free((void *)topicName);
            return 1;
        }
    }

    response_DeviceMQTTptr((char *)(message->payload), topicName);

    LOG_INFO("Audit log END --> on_device_agent_mqtt_message");
    return 1;
}


/*
*
* Function:      onDeviceMqttDisconnectFailure
* Description :  Function handle Device MQTT failure to disconnect event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onDeviceMqttDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);
    LOG_INFO("Audit log START --> onDeviceMqttDisconnectFailure");

    LOG_ERROR("Disconnect failed, rc %d   --> onDeviceMqttDisconnectFailure", response->code);
    idevice_agent_disc_finished = 1;

    LOG_INFO("Audit log END --> onDeviceMqttDisconnectFailure");
}


/*
*
* Function:      onDeviceMqttDisconnect
* Description :  Function handle Device MQTT  disconnect event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_successData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onDeviceMqttDisconnect(void* context, MQTTAsync_successData* response)
{
    (void) (context);
    (void) (response);

    LOG_INFO("Audit log START --> onDeviceMqttDisconnect");

    LOG_INFO("Device broker disconnected successfully --> onDeviceMqttDisconnect");
    idevice_agent_disc_finished = 1;

    LOG_INFO("Audit log END --> onDeviceMqttDisconnect");
}


/*
*
* Function:      onDeviceMqttSubscribe
* Description :  Function handle Device MQTT  subuscribe event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_successData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onDeviceMqttSubscribe(void* context, MQTTAsync_successData* response)
{
    (void) (context);
    (void) (response);

    LOG_INFO("Audit log START --> onDeviceMqttSubscribe");

    LOG_INFO("Subscribed to device broker successfully --> onDeviceMqttSubscribe");
    idevice_agent_Subscribed = 1;

    // Remove LWT message
    char* lwtMessage = generateLastWillMessage("Agent connected");
    PublishRespToDevice(NULL, MQTT_LWT_EVENT_TOPIC, lwtMessage);
	char agentDataDeviceDetails[512] = {'\0'};
	setabsolutePathUtil(agentDataDeviceDetails, AGENTDATA_DEVICE_DETAILS);
	char agentDataGatewayDetails[512] = {'\0'};
	setabsolutePathUtil(agentDataGatewayDetails, AGENTDATA_GATEWAY_DETAILS);
    if ((isfile(agentDataDeviceDetails) != 1) && (isfile(agentDataGatewayDetails) != 1)) {
        strcpy(g_aAgentState.rRegistrationInfo.szStatus, MQTT_DEVICE_REGISTRATION_NOT_REGISTERED);
        LOG_INFO("Device not registered. Sending state message --> InitGatewayMQTT");
        sendAgentStateMessage();
    }

    LOG_INFO("Audit log END --> onDeviceMqttSubscribe");

}


/*
*
* Function:      onDeviceMqttSubscribeFailure
* Description :  Function handle Device MQTT  subuscribtion failure event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onDeviceMqttSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);

    LOG_INFO("Audit log START --> onDeviceMqttSubscribeFailure");

    LOG_ERROR("Subscribe failed, rc %d  --> onDeviceMqttSubscribeFailure", response->code);
    idevice_agent_Finished = 1;

    LOG_INFO("Audit log END --> onDeviceMqttSubscribeFailure");
}


/*
*
* Function:      onDeviceMqttConnectFailure
* Description :  Function handle Device MQTT connection failure event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onDeviceMqttConnectFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);

    LOG_INFO("Audit log START --> onDeviceMqttConnectFailure");

    LOG_ERROR("Failed to connect to device broker,return code %d  [%s]--> onDeviceMqttConnectFailure", response->code, response->message);
    idevice_agent_Finished = 1;

    LOG_INFO("Audit log END --> onDeviceMqttConnectFailure");
}


/*
*
* Function:      onDeviceMqttConnect
* Description :  Function handle Device MQTT connection event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_successData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onDeviceMqttConnect(void* context, MQTTAsync_successData* response)
{
    (void) (response);

    LOG_INFO("Audit log START --> onDeviceMqttConnect");

    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;

    opts.onSuccess = onDeviceMqttSubscribe;
    opts.onFailure = onDeviceMqttSubscribeFailure;
    opts.context = client;

    LOG_INFO("Initiating subscription to device broker --> onDeviceMqttConnect");
    if ((rc = MQTTAsync_subscribe(client, MQTTBROKER_TOPIC, 0, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to start subscription to device broker, return code %d   --> onDeviceMqttConnect", rc);
        idevice_agent_Finished = 1;
    }

    LOG_INFO("Audit log END --> onDeviceMqttConnect");
}


/*
*
* Function:      initializeDeviceMqtt
* Description :  Function handle MQTT connect and send callback cmd/event
*
* Return value:  void
* Argument:      char* (connect address), char* (MQTTAsync ClientID), char* (connect topic),
                 void ((*response_ptrarg)(char*, char*) - callback function)
* Note:          MQTT  protocol specific  callback  utility
*
*/
void initializeDeviceMqtt(const char* address, const char* ClientID, const char* topic, void (*response_ptrarg)(char*, char*))
{
    (void) (topic);

    LOG_INFO("Audit log START --> initializeDeviceMqtt");
    MQTTAsync_create(&deviceClient, address, ClientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    idevice_agent_Finished = 0;

    response_DeviceMQTTptr = response_ptrarg; // function for callback

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
    MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;

    int rc;
    LOG_INFO("Initiating callbacks to device broker --> initializeDeviceMqtt");
    if ((rc = MQTTAsync_setCallbacks(deviceClient, NULL, NULL, on_device_agent_mqtt_message, NULL)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to set callbacks to device broker, return code %d --> initializeDeviceMqtt", rc);
        return;
    }

    wopts.topicName = MQTT_LWT_EVENT_TOPIC;
    wopts.message = generateLastWillMessage("Agent terminated ungracefully");
    wopts.retained = 1;
    wopts.qos = 2;

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    conn_opts.will = &wopts;
    conn_opts.automaticReconnect = 1;

    conn_opts.onSuccess = onDeviceMqttConnect;
    conn_opts.onFailure = onDeviceMqttConnectFailure;
    conn_opts.context = deviceClient;
    conn_opts.ssl = &sslopts;
    conn_opts.ssl->trustStore = g_deviceBrokerCaCert;
    conn_opts.ssl->keyStore = g_deviceBrokerCert;
    conn_opts.ssl->privateKey = g_deviceBrokerPrivateKey;
    conn_opts.ssl->verify = 1;

    LOG_INFO("Initiating connection to device broker --> initializeDeviceMqtt");
    if ((rc = MQTTAsync_connect(deviceClient, &conn_opts)) != MQTTASYNC_SUCCESS) {
        LOG_ERROR("Failed to connect to Device broker, return code %d --> initializeDeviceMqtt", rc);
        exit(-1);
    }

    LOG_INFO("Audit log logical END --> initializeDeviceMqtt");
    // Handle Thread safe publish to device
    while (idevice_agent_Finished != 1)
    {
        #ifdef _WIN32
            Sleep(100);
        #else
            sleep(1);
        #endif
    }
}

char* generateLastWillMessage(const char *willMsg)
{
    cJSON* cjMessageToDeviceHeader = generateDeviceHeaderForEvent(NULL, "lwt", "agent", "eventIn", NULL);
    cJSON* cjMessage = cJSON_CreateObject();
    cJSON* cjMessagePayload = cJSON_CreateObject();
    cJSON_AddItemToObject(cjMessage, "header", cjMessageToDeviceHeader);
    cJSON* cjLwtMessage = cJSON_CreateString(willMsg);
    cJSON_AddItemToObject(cjMessagePayload, "message", cjLwtMessage);
    cJSON_AddItemToObject(cjMessage, "payload", cjMessagePayload);
    char* lwtMessage = cJSON_PrintUnformatted(cjMessage);
    cJSON_Delete(cjMessage);
    return lwtMessage;
}

//------------------------------------------------------------------------------------------------------------------------------

//                                            Device MQTT  Client  Code end 

//------------------------------------------------------------------------------------------------------------------------------



//******************************************************************************************************************************

//------------------------------------------------------------------------------------------------------------------------------

//                                            Gatway MQTT  Client  Code start 
 
//------------------------------------------------------------------------------------------------------------------------------

/*
* Function:      onGatewayMqttDisconnectFailure
* Description :  Function handle Gateway MQTT failure to disconnect event
*
* Return value:  void
* Argument:      void* (MQTT context), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onGatewayMqttDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);

    LOG_INFO("Audit log START --> onGatewayMqttDisconnectFailure");
    status_ptrmqtt(GFAILTODISCONNECT);
    LOG_ERROR("Gateway broker disconnect failed, rc %d   --> onGatewayMqttDisconnectFailure", response->code);
    igateway_agent_disc_finished = 1;
    LOG_INFO("Audit log END --> onGatewayMqttDisconnectFailure");
}


/*
*
* Function:      onGatewayMqttDisconnect
* Description :  Function handle Gateway MQTT  disconnect event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_successData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onGatewayMqttDisconnect(void* context, MQTTAsync_successData* response)
{
    (void) (context);
    (void) (response);

    LOG_INFO("Audit log START --> onGatewayMqttDisconnect");
    status_ptrmqtt(GDISCONNECTED);
    LOG_INFO("Gateway broker disconnected successfully --> onGatewayMqttDisconnect");
    igateway_agent_disc_finished = 1;

    //If agent is suspended because of suspend command from device, exit the gateway thread
    //so that automatic reconnection is disabled
    if ( strcmp(g_aAgentState.szAgentStatus, MQTT_AGENT_STATUS_SUSPENDED) == 0)
    {
        igateway_agent_Finished = 1;
    }

    //Put agent in suspend mode and let the thread continue so that automatic reconnection 
    //could work
    //strcpy(g_aAgentState.szAgentStatus, MQTT_AGENT_STATUS_SUSPENDED);
    LOG_INFO("Enabling Offline Mode");
    offlineMode = 1;

    LOG_INFO("Audit log END --> onGatewayMqttDisconnect");
}

/*
*
* Function:      onGatewayMqttSubscribe
* Description :  Function handle Gateway MQTT  subuscribe event
*
* Return value:  void
* Argument:      void* (MQTT context), MQTTAsync_successData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onGatewayMqttSubscribe(void* context, MQTTAsync_successData* response)
{
    (void) (response);
    LOG_INFO("Audit log START --> onGatewayMqttSubscribe");
    gatewayClient = (MQTTAsync)context;

    LOG_INFO("Subscribed to gateway broker successfully --> onGatewayMqttSubscribe");
    igateway_agent_Subscribed = 1;

    LOG_INFO("Audit log END --> onGatewayMqttSubscribe");
    
    /* Check Agentdata\Agentstate.json. If state is suspended signal the event */
    char agentStateFilePath[512] = {'\0'};
    setabsolutePathUtil(agentStateFilePath, AGENTDATA_STATE_FILE_PATH);
    const char* pszAgentStateInfo = readFromFile(agentStateFilePath);
    if (NULL == pszAgentStateInfo)
    {
        LOG_ERROR("readFromFile(AGENTDATA_STATE_FILE_PATH):Could not read from Agent state Json file");
    }
    else
    {    
        const char* pszAgentState = getJsonData(pszAgentStateInfo, "state");
        if (pszAgentState == NULL)
        {
            LOG_ERROR("Could not find the Agent sate from Agent state Json file");
            return;
        }

        if (strcmp(pszAgentState, "suspend") == 0)
        {    
        #ifdef _WIN32
            SetEvent(g_hRestartSuspendEvent);  //Signal the event to unblock WaitForSingleObject after gateway connection is established and agent is running after service restart
        #else
            pthread_cond_signal(&g_SuspendAgentCond);
        #endif
        }
    }
}

/*
* Function:      onGatewayMqttSubscribeFailure
* Description :  Function handle gateway MQTT  subuscribtion failure event
*
* Return value:  void
* Argument:      void* (MQTT context), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onGatewayMqttSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);
    LOG_INFO("Audit log START --> onGatewayMqttSubscribeFailure");
    status_ptrmqtt(GFAILTOSUB);
    LOG_ERROR("Subscribe failed, rc %d  --> onGatewayMqttSubscribeFailure", response->code);
    igateway_agent_Finished = 1;

    LOG_INFO("Audit log END --> onGatewayMqttSubscribeFailure");
}

/*
*
* Function:      onGatewayMqttConnectFailure
* Description :  Function handle gateway MQTT connection failure event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onGatewayMqttConnectFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);
    LOG_INFO("Audit log START --> onGatewayMqttConnectFailure");
    status_ptrmqtt(GFAILTOCONNECT);
    LOG_ERROR("Failed to connect to gateway ,return code %d  --> onGatewayMqttConnectFailure", response->code);

    LOG_INFO("Audit log END --> onGatewayMqttConnectFailure");
}

/*
* Function:      onGatewayMqttConnect
* Description :  Function handle gateway MQTT connection event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_successData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onGatewayMqttConnect(void* context, MQTTAsync_successData* response)
{
    (void) (response);

    LOG_INFO("Audit log START --> onGatewayMqttConnect");

    status_ptrmqtt(GCONNECTED);
   
    gatewayClient = (MQTTAsync)context;

    sendFirstMessageToGateway();

    LOG_INFO("Disabling Offline Mode");
    offlineMode = 0;
    LOG_INFO("Message '%s' --> onGatewayMqttConnect \n", sGatewayInitializationPayload);
    LOG_INFO("Audit log END --> onGatewayMqttConnect");
}

/*
* Function:      onConnectionLost
* Description :  Function handle gateway MQTT  connection failure event
*
* Return value:  void
* Argument:      void* (MQTTcontext), MQTTAsync_failureData* (MQTT response)
* Note:           MQTT  protocol specific  callback  utility
*
*/
void onConnectionLost(void* context, char* cause)
{
    (void) (context);
    (void) (cause);
    LOG_INFO("Connection to Gateway Lost  --> onConnectionLost");
    LOG_INFO("Enabling Offline Mode");
    offlineMode = 1;
    status_ptrmqtt(GDISCONNECTED);
}

/*
*
* Function:      onGatewayMqttReConnect
* Description :  Function called when successfully reconnected to Gateway
*
* Return value:  void
* Argument:      void* (Context), 
* Note:          
*
*/
void onGatewayMqttReConnect(void* vPContext, char* pszReason)
{
    (void) (vPContext);
    (void) (pszReason);

    LOG_INFO("Audit log START --> onGatewayMqttReConnect");
    LOG_INFO("Successfully reconnected to gateway --> onGatewayMqttReConnect");

    if (strcmp(g_aAgentState.szGatewayConnectionStatus, MQTT_CONNECTION_STATUS_NOTCONNECTED) == 0 )
    {
        status_ptrmqtt(GCONNECTED);

        sendFirstMessageToGateway();
    }
    LOG_INFO("Disabling Offline Mode");
    offlineMode = 0;
    LOG_INFO("Execute Requests Received During Offline Mode");
    executeOfflineRequests();
    LOG_INFO("Restore Agent To State Before Restart");
    restoreAgentStateAfterGatewayReconnect();
    LOG_INFO("Audit log END --> onGatewayMqttReConnect");
}

void clearGatewayMQTT()
{
    LOG_INFO("Audit Log Start ---> clearGatewayMQTT");
    int status = 0;
    status = MQTTAsync_disconnect(gatewayClient, NULL);
    if (status == MQTTASYNC_SUCCESS)
    {
        LOG_INFO("Disconnection has been Successful");
    }
    else
    {
        LOG_INFO("Disconnection was not Successful");
    }
    MQTTAsync_destroy(&gatewayClient);
    LOG_INFO("Audit Log End ---> clearGatewayMQTT");
}

/*
*
* Function:      initializeGatewayMqtt
* Description :  Function to intiate Gateway side MQTT client
*
* Return value:  void
* Argument:       char *(server address), char *( Client Identity), char *( initial topic), (function pointer to receive response)
* Note:           MQTT  protocol specific  user  utility
*
*/
void initializeGatewayMqtt(const char* sAddress, const char* sClientID, const char* sTopic, const char* sPayload, void (*response_ptrarg)(char*, char*), void(*status_ptrarg)(MQTTConnentState)) {

    LOG_INFO("Audit log START-- > %s", __func__);

    igateway_agent_Finished = 0;
    MQTTAsync_create(&gatewayClient, sAddress, sClientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    status_ptrmqtt = status_ptrarg;
    response_GatewayMQTTptr = response_ptrarg;
    bFlagGatewayPub = 0;
    char sTempbuf[ SIZE_512 ] = { '\0' };
    MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
    wopts.topicName = sTopic;
    sprintf( sTempbuf, "{\"header\":{ \"device-id\":\"%s\",\"name\":\"STATUS\", \"type\":\"STATUS\"}, \"message\":{ \"state\": \"DISCONNECTED\"}}", g_SdeviceID );
    wopts.message = sTempbuf;
    wopts.retained = 1;
    wopts.qos = 2;
    conn_opts.will = &wopts;

    ssl_opts.trustStore = g_deviceCaCert;
    ssl_opts.keyStore = g_deviceCert;
    ssl_opts.privateKey = g_privateKey;


    ssl_opts.enableServerCertAuth = 0;
    ssl_opts.disableDefaultTrustStore = 1;
    ssl_opts.enableServerCertAuth = 0;
    ssl_opts.verify = 0;

    conn_opts.ssl = &ssl_opts;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    conn_opts.onSuccess = onGatewayMqttConnect;
    conn_opts.onFailure = onGatewayMqttConnectFailure;
    conn_opts.context = gatewayClient;

    conn_opts.automaticReconnect = 1;

    strcpy(sGatewayPublishTopic, sTopic);
    strcpy(sGatewayInitializationPayload, sPayload);

    int rc;
    LOG_INFO("Initiating callbacks to gateway broker --> initializeGatewayMqtt");

    if ((rc = MQTTAsync_setCallbacks(gatewayClient, NULL, onConnectionLost, on_agent_gateway_mqtt_message, NULL)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to set callbacks to gateway broker, return code %d  --> initializeGatewayMqtt", rc);
        return;
    }

    if ((rc = MQTTAsync_setConnected(gatewayClient, NULL, onGatewayMqttReConnect)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to set connected callback to gateway broker, return code %d  --> initializeGatewayMqtt", rc);
        return;
    }
    else
    {
        LOG_INFO("Successfully set connected callback to gateway broker, return code %d  --> initializeGatewayMqtt", rc);
    }

    LOG_INFO("Initiating connection to gateway broker --> initializeGatewayMqtt");
    if ((rc = MQTTAsync_connect(gatewayClient, &conn_opts)) != MQTTASYNC_SUCCESS) {
        LOG_ERROR("Failed to connect to gateway broker, return code %d  --> initializeGatewayMqtt", rc);
        strcpy(g_aAgentState.szGatewayConnectionStatus, MQTT_CONNECTION_STATUS_NOTCONNECTED);
        return;
    }
    

    while (igateway_agent_Finished != 1) {
        if (bFlagGatewayPub == 1)
        {
            bFlagGatewayPub = 0;
            publishToGateway(NULL, pszTopic, pszPayload);
        }
        #ifdef _WIN32
            Sleep(100);
        #else
            usleep(1000);
        #endif
    }

    
    LOG_INFO("Exiting gateway thread. --> initializeGatewayMqtt");

    LOG_INFO("Audit log END-- > %s", __func__);
}

/*
*
* Function:      onSendToGateway
* Description :  Function publish message to device
*
* Return value:  void
* Argument:     void* context(MQTT context), MQTTAsync_successData* response(MQTT response)
* Note:         MQTT  protocol specific   utility
*
*/
void onSendToGateway(void* context, MQTTAsync_successData* response)
{
    (void) (context);
    (void) (response);

    LOG_INFO("Message sent to gateway successfully --> onSendToGateway");
}


/*
*
* Function:      onSendToGatewayFailure
* Description :  Function for  fail callback
*
* Return value:  Void
* Argument:     void* context(MQTT context), MQTTAsync_failureData* response(MQTT response)
* Note:          MQTT  protocol specific   utility
*
*/
void onSendToGatewayFailure(void* context, MQTTAsync_failureData* response)
{
    (void) (context);

    LOG_INFO("Audit log START --> onSendToGatewayFailure");

    LOG_ERROR("Send message to gateway failed, rc %d %s --> onSendToGatewayFailure", response->code, response->message);

    LOG_INFO("Audit log END --> onSendToGatewayFailure");
}
/*
*
* Function:      publishToGatewayUpdate
* Description :  Function to publish message to gateway
*
* Return value:  Void
* Argument:      char *(topic), char * (payload)
* Note:          MQTT specific  user utility async
*
*/
void publishToGatewayUpdate(const char* sTopic, const char* sMessage)
{
    LOG_INFO("Audit log START--> %s", __func__);
    pszTopic = (char*)malloc((strlen(sTopic)) * sizeof(char) + 1);
    memset(pszTopic, '\0', sizeof(pszTopic));
    strcpy(pszTopic, sTopic);
    pszPayload = (char*)malloc((strlen(sMessage)) * sizeof(char) + 1);
    memset(pszPayload, '\0', sizeof(pszPayload));
    strcpy(pszPayload, sMessage);

    bFlagGatewayPub = 1;
    LOG_INFO("Audit log END--> %s", __func__);

}


/*
*
* Function:      publishToGateway
* Description :  Function to publish message to gateway
*
* Return value:  Void
* Argument:      char *(topic), char * (payload)
* Note:          MQTT specific  user utility
*
*/
void publishToGateway(char *pszTtl, const char *sTopic, char *sMessage)
{

    LOG_INFO("Audit log START --> publishToGateway");
    
    LOG_INFO("Received following message to be published to gateway: %s --> publishToGateway", sMessage);

    if ((pszTtl != NULL) && (strlen(pszTtl) > 0))
    {
        LOG_INFO("TTL value from Gateway Response = [%s]", pszTtl);
        if (hasTtlExpired(pszTtl))
        {
            LOG_ERROR("TTL expired for response of gateway request");
            return;
        }
    }

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    opts.onSuccess = onSendToGateway;
    opts.onFailure = onSendToGatewayFailure;
    opts.context = gatewayClient;

    char sGatewayTopic[SIZE_512];
    strcpy(sGatewayTopic, sTopic);
    pubmsg.payload = sMessage;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 2;
    pubmsg.retained = 0;

    int rc = 0;
    LOG_INFO("Sending message to gateway on topic: %s --> publishToGateway", sGatewayTopic);
    LOG_INFO("Sending message to gateway: %s --> publishToGateway", sMessage);
    if ((rc = MQTTAsync_sendMessage(gatewayClient, sGatewayTopic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to sendMessage, return code %d   --> publishToGateway", rc);
    }
      

    LOG_INFO("Audit log END --> publishToGateway");
}

/*
*
* Function:      on_agent_gateway_mqtt_message
* Description :  Function publish message from agent to gateway
*
* Return value:  Void
* Argument:     void* context(MQTT context), char * (topic Name), int (topic size), MQTTAsync_message *( MQTT message)
* Note:         MQTT  protocol specific  callback  utility
*
*/
int on_agent_gateway_mqtt_message(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    (void) (context);
    (void) (topicName);
    (void) (topicLen);

    LOG_INFO("Audit log START --> on_agent_gateway_mqtt_message");

    LOG_INFO("Received message from gateway payload: %s, topic: %s --> on_agent_gateway_mqtt_message", (char *)message->payload, topicName);

    response_GatewayMQTTptr((char *)message->payload, topicName);

    LOG_INFO("Audit log END--> on_agent_gateway_mqtt_message");
    return 1;
}

//------------------------------------------------------------------------------------------------------------------------------

//                                            Gatway MQTT  Client  Code ends 

//------------------------------------------------------------------------------------------------------------------------------



char* GetGetwayServerMqtt( const char *filePath)
{

    FILE* fp;
    long lSize;
    char* sBuffer;
    fp = fopen(filePath, "rb");
    if (!fp)
    {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);
    /* allocate memory for entire content */
    sBuffer = calloc(1, lSize + 1);
    if (!sBuffer)
    {
        fclose(fp);
        return NULL;
    }
    /* copy the file into the buffer */
    if ((fread(sBuffer, lSize, 1, fp)) != 1)
    {
        fclose(fp);
        free(sBuffer);
        return NULL;
    }
    fclose(fp);
    return sBuffer;
   
}
void StopGatewayMQTT()
{
    LOG_INFO("====Stopped MQTT gateway====");
    igateway_agent_Finished = 1;

}

void sendFirstMessageToGateway(void)
{
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    int rc;

    pubmsg.payload = sGatewayInitializationPayload;
    pubmsg.payloadlen = (int)strlen(sGatewayInitializationPayload);
    pubmsg.qos = 2;
    pubmsg.retained = 0;

    LOG_INFO("Sending success message to gateway to make device active %s--> %s", sGatewayInitializationPayload, __func__);

    if ((rc = MQTTAsync_sendMessage(gatewayClient, sGatewayPublishTopic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to Send message to gateway to make device active, return code %d --> %s", rc, __func__);
    }
    opts.onSuccess = onGatewayMqttSubscribe;
    opts.onFailure = onGatewayMqttSubscribeFailure;
    opts.context = gatewayClient;

    sprintf(sGatewaySubscribeTopic, "/CARE/%s/#", g_SdeviceID);
    LOG_INFO("Initiating subscription to gateway on topic %s --> %s", sGatewaySubscribeTopic, __func__);
    if ((rc = MQTTAsync_subscribe(gatewayClient, sGatewaySubscribeTopic, 0, &opts)) != MQTTASYNC_SUCCESS)
    {
        LOG_ERROR("Failed to start subscription to gateway, return code %d   --> %s", rc, __func__);
        igateway_agent_Finished = 1;
    }
}
