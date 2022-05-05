'use strict';

// id of the mesh model standard
var ModelID = {
    UNKNOWN: 0,
    GEN_ONOFF_CLI: 0x1001,
    GEN_LEVEL_CLI: 0x1003,
};

// opcode of the mesh model standard
var OpCode = {
    UNKNOWN: 0,
    // for model sig onoff
    ONOFF_GET: 0x8201,
    ONOFF_SET_ACK: 0x8202,
    ONOFF_SET: 0x8203,
    ONOFF_STATUS: 0x8204,
    
    // for model sig level
    LEVEL_SET_ACK: 0x8206,
    LEVEL_SET: 0x8207,
    LEVEL_GET: 0x8208,
    LEVEL_DELTA_SET_ACK: 0x8209,
    LEVEL_DELTA_SET: 0x820a,
    LEVEL_MOVE_SET_ACK: 0x820b,
    LEVEL_MOVE_SET: 0x820c,
};

// child_cls extends parent_cls for Ecmascript E5
function inherits(child_cls, parent_cls, child) {
    var F = function () {};
    F.prototype = parent_cls.prototype;
    child_cls.prototype = new F();
    child_cls.prototype.constructor = child_cls;
    parent_cls.call(child);
}

// implements for base class of the mesh model
function model_base() {
    this.model_id = ModelID.UNKNOWN;
    this.opcodes = [];

    // get op instance by the opcode from mesh model
    this.get_opcode = function(opcode) {
        for (var i = 0; i < this.opcodes.length; i++) {
            var codes = this.opcodes[i].codes;
            for (var j = 0; j < codes.length; j++) {
                if (codes[j] == opcode) {
                    var op = new this.opcodes[i].opclass();
                    if (op) {
                        // add default attribute for op instance
                        op.opcode   = opcode;
                        op.model_id = this.model_id;
                    }
                    return op;
                }
            }
        }
    }

    // update tid value after dev.send_to_device
    this.tid_inc = function(dev) {
        var tid_key = "tid_0x" + this.model_id.toString(16);
        // get current tid value by tid_key
        var tid = dev.value.get(tid_key);

        tid = tid >= 0xff ? 0 : tid++;
        // update tid value
        dev.value.set(tid_key, tid);
    }
}

// implements for generic onoff cli model
function model_sig_onoff() {
    // extends the model_base class
    inherits(model_sig_onoff, model_base, this);

    this.model_id = ModelID.GEN_ONOFF_CLI;
    this.opcodes = [
        {
            codes: [OpCode.ONOFF_GET],
            opclass: function() {
                // encode to ONOFF_GET mesh protocol
                this.encode = function(dev) {
                    var data = [];
            
                    data[0] = (this.opcode & 0xff00) >> 8;
                    data[1] = (this.opcode & 0xff);
                
                    return data;
                }
            }
        },
        {
            codes: [OpCode.ONOFF_SET_ACK, OpCode.ONOFF_SET],
            opclass: function() {
                this.onoff_status = 0;
                this.trans = 0;
                this.delay = 1;
                // encode to ONOFF_SET_ACK/ONOFF_SET mesh protocol
                this.encode = function(dev) {
                    var data = [];
                    var tid_key = "tid_0x" + this.model_id.toString(16);  
                    var tid = dev.value.get(tid_key);
            
                    data[0] = (this.opcode & 0xff00) >> 8;
                    data[1] = (this.opcode & 0xff);
                    data[2] = this.onoff_status;
                    data[3] = tid;
                    console.log("tid=" + tid);
            
                    if (this.trans) {
                        data[4] = this.trans;
                        data[5] = this.delay;
                    }
                
                    return data;
                }
            }
        }, 
        {
            codes: [OpCode.ONOFF_STATUS],
            opclass: function() {
                this.min_len = 1;
                this.onoff_status = 0;
                // decode from ONOFF_STATUS mesh protocol
                this.decode = function(dev, data) {
                    if (data && data.length >= this.min_len) {
                        this.onoff_status = data[0];
                        return 0;
                    }
                    return -1;
                }
            }
        }
    ];
}

// instance of the mesh management
var Mesh = {
    // register mesh models
    mesh_models: [
        new model_sig_onoff()
    ],
    // get mesh model instance by ModelID.xxx
    get_model_by_id: function(model_id) {
        for (var i = 0; i < this.mesh_models.length; i++) {
            var model = this.mesh_models[i];
            if (model.model_id == model_id)
                return model;
        }
    
        return null;
    },
    // get mesh model instance by OpCode.xxx
    get_model_by_opcode: function(opcode) {   
        for (var i = 0; i < this.mesh_models.length; i++) {
            var model = this.mesh_models[i];
            for (var j = 0; j < model.opcodes.length; j++) {
                var codes = model.opcodes[j].codes;
                for (var k = 0; k < codes.length; k++) {
                    if (codes[k] == opcode) {
                        return model;
                    }
                }
            }
        }
    
        return null;
    },

    // get op instance by OpCode.xxx
    get_opcode: function(opcode) {
        var model = this.get_model_by_opcode(opcode);
        return model ? model.get_opcode(opcode) : null;
    }
}

// cloud_to_device和device_to_cloud函数定义及入参详细描述请参考《分布式物模型用户开发手册》 物模型执行器小节说明
var runner = {
    /**
     * convert the IoT cloud platform object model protocol into a device 
     * protocol and send it to the underlying physical link
     * example=> {"powerstate":1}->8202 01 01
     * @param dev the virtual terminal sub device 
     * @param cloud_data downstream of the IoT cloud platform
     * @return 0/-1
     */ 
    cloud_to_device: function (dev, cloud_data) {
        // cloud_data may be a json-string, convert to a json object
        var json = JSON.parse(cloud_data);
        if (json) {
            var val = json['powerstate'];
            if (val != null) {
                var model = Mesh.get_model_by_id(ModelID.GEN_ONOFF_CLI);
                var op = model.get_opcode(OpCode.ONOFF_SET_ACK);
                op.onoff_status = val;
                op.trans = 0;
                op.delay = 3;
                // convert to the device protocol
                var data = op.encode(dev);
                // send device protocol to the underlying physical link
                var rc = dev.send_to_device(data);
                if (rc == 0) {
                    // update the tid value as needed
                    model.tid_inc(dev);
                }
                return rc;
            }
        }
        
        return -1;
    },
    /**
     * convert the device protocol to the IoT cloud platform object
     * model protocol and send it to the cloud
     * example=> 8204 01->{"powerstate":1}
     * @param dev the virtual terminal sub device 
     * @param dev_data upstream of the sub device
     * @return 0/-1
     */
    device_to_cloud: function (dev, dev_data) {
        if (!(dev && dev_data && dev_data.opcode && dev_data.data)) {
            console.log('params is UNKNOWN');
            return null;
        }
        // get op instance by the opcode
        var op = Mesh.get_opcode(dev_data.opcode);
        if (op) {
            // convert to protocol of the IoT cloud platform object model protocol
            var rc = op.decode(dev, dev_data.data);
            if (rc == 0) {
                var json = new Object();
    
                json["powerstate"] = op.onoff_status;
                // convert to json string from a json object
                var data = JSON.stringify(json);
                // send IoT cloud object model protocol to the cloud
                return dev.send_to_cloud(data);
            }
        }

        return -1;
    }
}





