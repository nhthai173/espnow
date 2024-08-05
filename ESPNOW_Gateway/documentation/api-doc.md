## API Documentation

This documentation provides an overview of the endpoints available on the Gateway server. The API is designed to manage devices and automations, providing various functionalities for interacting with connected devices.

### Base URL

All endpoints are relative to the base URL: `http://<ESP_IP_ADDRESS>`

### Endpoints

- [GET /](#get-)
- [GET /device](#get-device)
- [GET /devices](#get-devices)
- [GET /pair](#get-pair)
- [POST /pair](#post-pair)
- [POST /unpair](#post-unpair)
- [POST /command](#post-command)
- [POST /restart](#post-restart)
- [POST /sync](#post-sync)
- [POST /auto/enable](#post-autoenable)
- [POST /auto/remove](#post-autoremove)
- [POST /auto/remove-all](#post-autoremove-all)
- [GET /auto/get](#get-autoget)
- [POST /auto/add-update](#post-autoadd-update)
- [Not Found](#not-found)

---

### GET `/`

- **Method:** `GET`
- **Description:** Returns a simple "Hello World" message.
- **Response:**
  - **Status Code:** `200 OK`
  - **Content-Type:** `text/plain`
  - **Body:** `"Hello World"`

---

### GET `/device`

- **Method:** `GET`
- **Description:** Retrieves information about a specific device using its ID.
- **Query Parameters:**
  - `id` (required): The unique identifier of the device.
- **Response:**

  - **Status Code:**
    - `200 OK` if the device is found.
    - `400 Bad Request` if the `id` parameter is missing.
    - `404 Not Found` if the device does not exist.
  - **Content-Type:** `application/json`
  - **Body:** JSON representation of the device details if found.

  **Example JSON Response:**

  ```json
  {
    "did": "6474413",
    "mac": "c4:5b:be:62:ca:ad",
    "online": "true",
    "last_online": "1722841599",
    "id": "6474413",
    "name": "Node2",
    "type": "ESP8266",
    "model": "Smart Door",
    "fw_ver": "2",
    "support": "set_prop,get_prop,get_props",
    "props_name": "r1,r2,r3",
    "r1": "false",
    "r2": "false",
    "r3": "false"
  }
  ```

---

### GET `/devices`

- **Method:** `POST`
- **Description:** Retrieves a list of all connected devices in JSON format.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON array of devices.
  - **Headers:**
    - `Access-Control-Allow-Origin: *`

  **Example JSON Response:**

  ```json
  [
    {
      "did": "5922554",
      "mac": "40:91:51:5a:5e:fa",
      "online": "true",
      "last_online": "1722841599",
      "id": "5922554",
      "name": "Node1",
      "type": "ESP8266",
      "model": "security-board",
      "fw_ver": "1",
      "support": "get_props,get_prop,set_prop",
      "props_name": "led,button",
      "led": "false",
      "button": "idle"
    },
    {
      "did": "6474413",
      "mac": "c4:5b:be:62:ca:ad",
      "online": "true",
      "last_online": "1722841599",
      "id": "6474413",
      "name": "Node2",
      "type": "ESP8266",
      "model": "Smart Door",
      "fw_ver": "2",
      "support": "set_prop,get_prop,get_props",
      "props_name": "r1,r2,r3",
      "r1": "false",
      "r2": "false",
      "r3": "false"
    }
  ]
  ```

---

### GET `/pair`

- **Method:** `GET`
- **Description:** Get the current pairing status.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON response of pairing details.

  **Example JSON Response:**

  In pairing mode with 12 seconds remaining:

  ```json
  {
    "success": true,
    "remaining": 12,
    "timeout": 30
  }
  ```
  
    Not in pairing mode:
    
    ```json
    {
        "success": true,
        "remaining": -1,
        "timeout": 30
    }
    ```
---

### POST `/pair`

- **Method:** `POST`
- **Description:** Manages the pairing process for devices. It can start, stop, or return the current pairing status.
- **Query Parameters:**
  - `start` (optional): Start the pairing process.
  - `stop` (optional): Stop the pairing process.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the operation and pairing details.

  **Example JSON Response:**

  In pairing mode with 12 seconds remaining:

  ```json
  {
    "success": true,
    "remaining": 12,
    "timeout": 30
  }
  ```
  Pairing stopped:

  ```json
  {
    "success": true,
  }
  ```

  Not in pairing mode:

    ```json
    {
        "success": true,
        "remaining": -1,
        "timeout": 30
    }
    ```

---

### POST `/unpair`

- **Method:** `GET`
- **Description:** Unpairs a device based on its ID.
- **Query Parameters:**
  - `id` (required): The unique identifier of the device to unpair.
- **Response:**

  - **Status Code:**
    - `200 OK` if the device is successfully unpaired.
    - `400 Bad Request` if the `id` parameter is missing.
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the operation.

  **Example JSON Response:**

  ```json
  {
    "success": true
  }
  ```
---

### POST `/command`

- **Method:** `POST`
- **Description:** Sends a command to a specified device.
- **Query Parameters:**
  - `id` (required): The unique identifier of the device.
  - `action` (required): The action or command to perform.
  - `params` (optional): Additional parameters for the command.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the command and any message returned.

  **Example JSON Response:**

  Send a command to a device and receive a success message:

  ```json
  {
    "success": true,
    "message": ""
  }
  ```
  Send a command to a device and receive error message from device:

  ```json
  {
    "success": false,
    "message": "Error from device description"
  }
  ```

---

### POST `/restart`

- **Method:** `POST`
- **Description:** Restarts a specific device or the gateway itself.
- **Query Parameters:**
  - `id` (optional): The unique identifier of the device to restart. If not provided, the entire gateway is restarted.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the restart operation.

  **Example JSON Response:**

  ```json
  {
    "success": true,
    "message": "restarting gateway"
  }
  ```
---

### POST `/sync`

- **Method:** `POST`
- **Description:** Synchronizes a specific device or all devices.
- **Query Parameters:**
  - `id` (optional): The unique identifier of the device to sync. If not provided, all devices are synchronized.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the sync operation.

  **Example JSON Response:**

  ```json
  {
    "success": true,
    "message": "All devices synchronized"
  }
  ```
---

### POST `/auto/enable`

- **Method:** `POST`
- **Description:** Enables or disables a specific automation.
- **Query Parameters:**
  - `id` (required): The unique identifier of the automation.
  - `disable` (optional): If present, disables the automation; otherwise, it enables the automation.
- **Response:**

  - **Status Code:**
    - `200 OK` if the operation is successful.
    - `400 Bad Request` if the `id` parameter is missing or the automation is not found.
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the operation.

  **Example JSON Response:**

  ```json
  {
    "success": true
  }
  ```
---

### POST `/auto/remove`

- **Method:** `POST`
- **Description:** Removes a specific automation.
- **Query Parameters:**
  - `id` (required): The unique identifier of the automation to remove.
- **Response:**

  - **Status Code:**
    - `200 OK` if the automation is successfully removed.
    - `400 Bad Request` if the `id` parameter is missing or the automation is not found.
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the operation.

  **Example JSON Response:**

  ```json
  {
    "success": true
  }
  ```
---

### POST `/auto/remove-all`

- **Method:** `POST`
- **Description:** Removes all automations.
- **Response:**
  - **Status Code:** `200 OK`
  - **Content-Type:** `text/plain`
  - **Body:** `"success"`

---

### GET `/auto/get`

- **Method:** `GET`
- **Description:** Retrieves a list of all automations in JSON format.
- **Response:**

  - **Status Code:** `200 OK`
  - **Content-Type:** `application/json`
  - **Body:** JSON array of automations.

  **Example JSON Response:**

  ```json
  [
    {
      "id": 56567,
      "enabled": true,
      "name": "Toggle LED",
      "match": 0,
      "triggers": [
        {
          "type": 2,
          "first": "5922554",
          "second": "button",
          "third": "click"
        }
      ],
      "actions": [
        {
          "type": 2,
          "first": "6474413",
          "second": "r1",
          "third": "toggle"
        }
      ]
    }
  ]
  ```
---

### POST `/auto/add-update`

- **Method:** `POST`
- **Description:** Adds or updates an automation based on the request body.
- **Request Body:** raw string representing the automation details.
- **Query Parameters:**
  - `id` (optional): The unique identifier of the automation to update. If not provided, a new automation is created.
- **Response:**

  - **Status Code:**
    - `200 OK` if the automation is successfully added or updated.
    - `400 Bad Request` if the automation data is invalid.
  - **Content-Type:** `application/json`
  - **Body:** JSON response indicating the success of the operation and the automation ID.

  **Example JSON Response:**

  ```json
  {
    "success": true,
    "id": 1
  }
  ```
---

### Not Found

- **Description:** Handles requests to undefined routes.
- **Response:**
  - **Status Code:** `404 Not Found`
  - **Content-Type:** `text/plain`
  - **Body:** `"Not Found"`

---

