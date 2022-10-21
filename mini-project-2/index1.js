const express = require("express");
const mysql = require("mysql");
const dbconfig = require("./config/database.js");
const connection = mysql.createConnection(dbconfig);
const awsIoT = require("aws-iot-device-sdk");

let deviceConnectFlag = false;
const app = express();
const bodyParser = require("body-parser");

const device = awsIoT.device({
  keyPath: "config/IOT_2018870081.private.key",
  certPath: "config/IOT_2018870081.cert.pem",
  caPath: "config/root-CA.crt",
  clientId: "IOT_2018870081",
  host: "a3i5w95d4244ci-ats.iot.ap-northeast-2.amazonaws.com",
  keepalive: 10,
});

device.on("connect", (connack) => {
  console.log("Device Connect:", connack);
  deviceConnectFlag = true;
  device.publish(
    "topic/A",
    JSON.stringify({
      data: "Hello World!",
    }),
    {
      qos: 1,
    }
  );
  device.subscribe("serverRequest");
});

device.on("close", (err) => {
  console.log("Device CLose:" + err);
  deviceConnectFlag = false;
});

device.on("reconnect", () => {
  console.log("Device ReConnect");
  deviceConnectFlag = true;
});

device.on("offLine", () => {
  console.log("Device offline");
  deviceConnectFlag = false;
});

device.on("error", (err) => {
  console.log("Device ERR:" + err);
  deviceConnectFlag = false;
});

device.on("message", (topic, payload) => {
  console.log("Received Topic: " + topic);
  console.log("Received Message: " + payload);
});

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

app.get("/api", (req, res) => {
  res.json({
    result: true,
  });
});

app.get("/api/status", (req, res) => {
  res.json({
    result: true,
    message: "connect Success",
  });
});

app.get("/api/users", (req, res) => {
  connection.query("SELECT * FROM users", (err, rows) => {
    if (err) throw err;
    console.log("User Info if ", rows);
    res.json({
      result: true,
      data: rows,
    });
  });
});

app.get("/api/sensorlog", (req, res) => {
  connection.query("SELECT * FROM sensor_log ORDER BY id DESC", (err, rows) => {
    if (err) throw err;
    console.log("sensor_log ", rows);
    res.json({
      result: true,
      data: rows,
    });
  });
});

app.post("/api/sensor_logs", (req, res) => {
  const temp = req.body.temp;
  const humidity = req.body.humidity;

  connection.query(
    "insert into sensor_log (temp, humidity) values (?, ?)",
    [temp, humidity],
    (err) => {
      if (err) throw err;
      res.json({
        result: true,
      });
    }
  );
});

app.listen(3000, () => {
  console.log("Run Hello World on Port 3000");
});
