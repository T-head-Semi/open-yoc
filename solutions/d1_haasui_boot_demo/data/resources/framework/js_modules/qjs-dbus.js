import * as _dbus from 'dbus';
import util from 'util';
import events from 'events';

var Define = function(type, name) {

	var dataDef = {
		type: Signature(type)
	};

	if (name)
		dataDef.name = name;

	return dataDef;
};

var Signature = function(type) {
	if (type == 'Auto') {
		return 'v';
	} else if (type === String) {
		return 's';
	} else if (type === Number) {
		return 'd';
	} else if (type === Boolean) {
		return 'b';
	} else if (type === Array) {
		return 'av';
	} else if (type === Object) {
		return 'a{sv}';
	} 

	return 'v';
};

var nextTick = function(cb){
  setTimeout(function(){
    cb && cb();
  }, 0);
};

var ForEachAsync = function(arr, callback, complete) {
	function next(index, length) {
		var self = this;

		if (index >= length) {
			if (complete)
				complete.apply(this, [ true ]);

			return;
		}

		function _next(stop) {
			if (stop === false) {

				if (complete)
					complete.apply(this, [ false ]);
				
				return;
			}

			nextTick(function() {
				if (ret === false) {

					if (complete)
						complete.apply(this, [ false ]);

					return;
				}

				next.apply(self, [ index + 1, length ]);
			});
		}

		var ret = callback.apply(self, [ arr[index], index, arr, _next ]);

		if (ret != true)
			_next();
	}

	next.apply(this, [ 0, arr.length ]);
};

var Utils = {
  Define,
  Signature,
  ForEachAsync,
  nextTick
};

var Interface = function (bus, serviceName, objectPath, interfaceName, obj) {
  var self = this;

  self.bus = bus;
  self.serviceName = serviceName;
  self.objectPath = objectPath;
  self.interfaceName = interfaceName;
  self.object = obj;
};

util.inherits(Interface, events.EventEmitter);

Object.defineProperty(Interface.prototype, 'connected', {
  get: function () {
    return this.bus.connected;
  }
});

Interface.prototype.init = function (callback) {
  var self = this;

  // Initializing methods
  for (var methodName in self.object['method']) {

    self[methodName] = (function (method, signature) {

      return function () {
        var allArgs = Array.prototype.slice.call(arguments);
        var interfaceIn = self.object.method[method].in;
        var dbusArgs = allArgs.slice(0, interfaceIn.length);
        var restArgs = allArgs.slice(interfaceIn.length);
        var options = restArgs[0];
        var callback = restArgs[1];

        if (typeof options === 'function') {
          // No options were specified; only a callback.
          callback = options;
          options = {};
        }

        if (!options) {
          options = {};
        }
        if (!callback) {
          callback = function () { };
        }
        var timeout = options.timeout || -1;
        this[method].finish || null;
        this[method].error || null;

        Utils.nextTick(function () {
          if (!self.connected) {
            callback(new Error('Bus is no longer connected'));
            return;
          }

          try {
            self.bus.callMethod(self.bus.connection,
              self.serviceName,
              self.objectPath,
              self.interfaceName,
              method,
              signature,
              timeout,
              dbusArgs,
              callback);
          } catch (e) {
            callback(e);
          }
        });
      };
    })(methodName, self.object['method'][methodName]['in'].join('') || '');
  }

  // Initializing signal handler
  var signals = Object.keys(self.object['signal']);
  if (signals.length) {
    self.bus.registerSignalHandler(self.serviceName, self.objectPath, self.interfaceName, self, function () {

      if (callback)
        Utils.nextTick(callback);
    });

    return;
  }

  if (callback)
    Utils.nextTick(callback);
};

Interface.prototype.setProperty = function (propertyName, value, callback) {
  var self = this;

  if (!self.connected) {
    Utils.nextTick(function () {
      callback(new Error('Bus is no longer connected'));
    });
    return;
  }

  var propSig = self.object['property'][propertyName].type;

  self.bus.callMethod(self.bus.connection,
    self.serviceName,
    self.objectPath,
    'org.freedesktop.DBus.Properties',
    'Set',
    { type: 'ssv', concrete_type: 'ss' + propSig },
    -1,
    [self.interfaceName, propertyName, value],
    function (err) {

      if (callback)
        callback(err);
    });
};

Interface.prototype.getProperty = function (propertyName, callback) {
  var self = this;

  if (!self.connected) {
    Utils.nextTick(function () {
      callback(new Error('Bus is no longer connected'));
    });
    return;
  }

  self.bus.callMethod(self.bus.connection,
    self.serviceName,
    self.objectPath,
    'org.freedesktop.DBus.Properties',
    'Get',
    'ss',
    10000,
    [self.interfaceName, propertyName],
    function (err, value) {

      if (callback)
        callback(err, value);
    });
};

Interface.prototype.getProperties = function (callback) {
  var self = this;

  if (!self.connected) {
    Utils.nextTick(function () {
      callback(new Error('Bus is no longer connected'));
    });
    return;
  }

  self.bus.callMethod(self.bus.connection,
    self.serviceName,
    self.objectPath,
    'org.freedesktop.DBus.Properties',
    'GetAll',
    's',
    -1,
    [self.interfaceName],
    function (err, value) {

      if (callback)
        callback(err, value);
    });
};

function DBusError(name, message) {
	Error.call(this, message);
	this.message = message;
	this.dbusName = name;

	if (Error.captureStackTrace) {
		Error.captureStackTrace(this, 'DBusError');
	}
	else {
		Object.defineProperty(this, 'stack', { value: (new Error()).stack });
	}
}

util.inherits(DBusError, Error);

DBusError.prototype.toString = function() {
	return 'DBusError: ' + this.message;
};

var Bus = function(_dbus, DBus, busName) {
	var self = this;

	self._dbus = _dbus;
	self.DBus = DBus;
	self.name = busName;
	self.signalHandlers = {};
	self.signalEnable = false;
	self.interfaces = {};

	switch(busName) {
	case 'system':
		self.connection = _dbus.getBus(0);
		break;

	case 'session':
		self.connection = _dbus.getBus(1);
		break;
	}

	self.on('signal', function(uniqueBusName, sender, objectPath, interfaceName, signalName, args) {

		if (objectPath == '/org/freedesktop/DBus/Local' &&
			interfaceName == 'org.freedesktop.DBus.Local' &&
			signalName == 'Disconnected') {

			self.reconnect();

			return;
		}

		var signalHash = objectPath + ':' + interfaceName;

		if (self.signalHandlers[signalHash]) {
			var args = [ signalName ].concat(args);

			var interfaceObj = self.signalHandlers[signalHash];
			interfaceObj.emit.apply(interfaceObj, args);
		}
	});

	// Register signal handler of this connection
	self.DBus.signalHandlers[self.connection.uniqueName] = self;
	self.DBus.enableSignal(self.connection);
};

util.inherits(Bus, events.EventEmitter);

Object.defineProperty(Bus.prototype, 'connected', {
	get: function() {
		return this.connection !== null;
	}
});

Bus.prototype.disconnect = function(callback) {
	var self = this;

	delete self.DBus.signalHandlers[self.connection.uniqueName];

	self._dbus.releaseBus(self.connection);

	self.connection = null;

	if (callback)
  Utils.nextTick(callback);
};

Bus.prototype.reconnect = function(callback) {
	var self = this;

	if(self.connection) {
		delete self.DBus.signalHandlers[self.connection.uniqueName];

		self._dbus.releaseBus(self.connection);
	}

	switch(self.name) {
	case 'system':
		self.connection = self._dbus.getBus(0);
		break;

	case 'session':
		self.connection = self._dbus.getBus(1);
		break;
	}

	self.DBus.signalHandlers[self.connection.uniqueName] = self;

	// Reregister signal handler
	for (var hash in self.interfaces) {
		var iface = self.interfaces[hash];

		self.registerSignalHandler(iface.serviceName, iface.objectPath, iface.interfaceName, iface);
	}

	if (callback)
  Utils.nextTick(callback);
};

Bus.prototype.introspect = function(serviceName, objectPath, callback) {
	var self = this;

	if (!self.connected) {
		Utils.nextTick(function() {
			callback(new Error('Bus is no longer connected'));
		});
		return;
	}

	// Getting scheme of specific object
	self.callMethod(self.connection,
		serviceName,
		objectPath,
		'org.freedesktop.DBus.Introspectable',
		'Introspect',
		'',
		10000,
		[],
		function(err, introspect) {

			var obj = self._parseIntrospectSource(introspect);
			if (!obj) {
				if (callback)
					callback(new Error('No introspectable'));

				return;
			}

			if (callback)
				callback(err, obj);
		});
};

Bus.prototype._parseIntrospectSource = function(source) {
	return this._dbus.parseIntrospectSource.apply(this, [ source ]);
};

Bus.prototype.getInterface = function(serviceName, objectPath, interfaceName, callback) {
	var self = this;

	if (self.interfaces[serviceName + ':' + objectPath + ':' +interfaceName]) {
		if (callback)
    Utils.nextTick(function() {
				callback(null, self.interfaces[serviceName + ':' + objectPath + ':' +interfaceName]);
			});

		return;
	}

	self.introspect(serviceName, objectPath, function(err, obj) {
		if (err) {
			if (callback)
				callback(err);

			return;
		}

		if (!(interfaceName in obj)) {
			if (callback)
				callback(new Error('No such interface'));

			return;
		}

		// Create a interface object based on introspect
		var iface = new Interface(self, serviceName, objectPath, interfaceName, obj[interfaceName]);
		iface.init(function() {

			self.interfaces[serviceName + ':' + objectPath + ':' +interfaceName] = iface;

			if (callback)
				callback(null, iface);
		});
	});
};

Bus.prototype.registerSignalHandler = function(serviceName, objectPath, interfaceName, interfaceObj, callback) {
	var self = this;

	self.getUniqueServiceName(serviceName, function (err, uniqueName) {
		// Make a hash
		var signalHash = objectPath + ':' + interfaceName;
		self.signalHandlers[signalHash] = interfaceObj;

		// Register interface signal handler
		self.addSignalFilter(serviceName, objectPath, interfaceName, callback);
	});
};

Bus.prototype.setMaxMessageSize = function(size) {
	this._dbus.setMaxMessageSize(this.connection, size || 1024000);
};

Bus.prototype.getUniqueServiceName = function(serviceName, callback) {
	var self = this;

	self.callMethod(self.connection,
		'org.freedesktop.DBus',
		'/',
		'org.freedesktop.DBus',
		'GetNameOwner',
		's',
		-1,
		[serviceName],
		function(err, uniqueName) {
			callback(err, uniqueName);
		});
};

Bus.prototype.addSignalFilter = function(sender, objectPath, interfaceName, callback) {
	var self = this;

	// Initializing signal if it wasn't enabled before
	// if (!self.signalEnable) {
	// 	self.signalEnable = true;
	// 	self._dbus.addSignalFilter(self.connection, 'type=\'signal\'');
	// }

	self._dbus.addSignalFilter(self.connection, 'type=\'signal\',interface=\'' + interfaceName + '\',path=\'' + objectPath + '\'');

	Utils.nextTick(function() {
		if (callback)
			callback();
	});
};

Bus.prototype._sendMessageReply = function(message, value, type) {
	this._dbus.sendMessageReply(message, value, type);
};

Bus.prototype._sendErrorMessageReply = function(message, name, msg) {
	this._dbus.sendErrorMessageReply(message, name, msg);
};

function createError(name, message) {
	return new DBusError(name, message);
}

Bus.prototype.callMethod = function() {
	var args = Array.prototype.slice.call(arguments);
	args.push(createError);
	this._dbus.callMethod.apply(this, args);
};

var ServiceInterface = function(object, interfaceName) {
	var self = this;

	self.object = object;
	self.name = interfaceName;
	self.introspection = null;
	self.methods = {};
	self.properties = {};
	self.signals = {};
};

util.inherits(ServiceInterface, events.EventEmitter);

ServiceInterface.prototype.addMethod = function(method, opts, handler) {
	var self = this;

	var _opts = opts || {};
	var methodObj = {
		handler: handler
	};

	if (_opts['in']) {
		var argSignature = [];
		for (var index in _opts['in']) {
			argSignature.push(_opts['in'][index]);
		}

		if (argSignature.length)
			methodObj['in'] = argSignature;
	}

	if (_opts['out']) {
		methodObj['out'] = _opts['out'];
	}

	self.methods[method] = methodObj;

	return self;
};

ServiceInterface.prototype.addProperty = function(propName, opts) {
	var self = this;

	var _opts = opts || {};
	var propObj = {
		access: 'read',
		type: opts.type || 'v',
		getter: opts.getter || null,
		setter: opts.setter || null
	};

	if (_opts['setter']) {
		propObj['access'] = 'readwrite';
	}

	self.properties[propName] = propObj;

	return self;
};

ServiceInterface.prototype.addSignal = function(signalName, opts) {
	var self = this;

	if (!opts)
		return;

	self.signals[signalName] = opts;

	self.on(signalName, function() {
		var args = [ signalName ].concat(Array.prototype.slice.call(arguments));
		self.emitSignal.apply(this, args);
	});

	return self;
};

ServiceInterface.prototype.call = function(method, message, args) {
	var self = this;

	var member = self.methods[method];
	if (!member) {
		self.object.service.bus._sendErrorMessageReply(message, 'org.freedesktop.DBus.Error.UnknownMethod');
		return;
	}

	var inArgs = member['in'] || [];
	if (inArgs.length != args.length) {
		self.object.service.bus._sendErrorMessageReply(message, 'org.freedesktop.DBus.Error.InvalidArgs');
		return;
	}

	// Preparing callback
	args = Array.prototype.slice.call(args).concat([ function(err, value) {
		var type;

		// Error handling
		if (err) {
			var errorName = 'org.freedesktop.DBus.Error.Failed';
			var errorMessage = err.toString();
			if (err instanceof Error) {
				errorMessage = err.message;
				errorName = err.dbusName || 'org.freedesktop.DBus.Error.Failed';
			}
			self.object.service.bus._sendErrorMessageReply(message, errorName, errorMessage);

			return;
		}

		if (member.out)
			type = member.out.type || '';

		self.object.service.bus._sendMessageReply(message, value, type);
	} ]);

	member.handler.apply(this, args);
};

ServiceInterface.prototype.getProperty = function(propName, callback) {
	var self = this;
	var prop = self.properties[propName];
	if (!prop) {
		return false;
	}

	prop.getter.apply(this, [ function(err, value) {
		if (callback)
			callback(err, value);
	} ]);

	return true;
};

ServiceInterface.prototype.setProperty = function(propName, value, callback) {
	var self = this;
	var prop = self.properties[propName];
	if (!prop) {
		return false;
	}

	var args = [value];

	args.push(function(err) {
		// Completed
		callback(err);
	});

	prop.setter.apply(this, args);

	return true;
};

ServiceInterface.prototype.getProperties = function(callback) {
	var self = this;
	var properties = {};

	var props = Object.keys(self.properties);
	Utils.ForEachAsync(props, function(propName, index, arr, next) {

		// Getting property
		var prop = self.properties[propName];
		prop.getter(function(err, value) {
			if (err) {
					// TODO: What do we do if a property throws an error?
					// For now, just skip the property?
					return next();
			}
			properties[propName] = value;
			next();
		});

		return true;
	}, function() {
		if (callback)
			callback(null, properties);
	});
};

ServiceInterface.prototype.emitSignal = function() {
	var self = this;

	var service = self.object.service;
	if (!service.connected) {
		throw new Error('Service is no longer connected');
	}

	var conn = self.object.service.bus.connection;
	var objPath = self.object.path;
	var interfaceName = self.name;
	var signalName = arguments[0];
	var args = Array.prototype.slice.call(arguments);
	args.splice(0, 1);

	var signal = self.signals[signalName] || null;
	if (!signal)
		return;

	var signatures = [];
	for (var index in signal.types) {
		signatures.push(signal.types[index].type);
	}

	self.object.service.bus._dbus.emitSignal(conn, objPath, interfaceName, signalName, args, signatures);

	return self;
};

ServiceInterface.prototype.update = function() {
	var self = this;

	var introspection = [];

	introspection.push('<interface name="' + self.name + '">');

	// Methods
	for (var methodName in self.methods) {
		var method = self.methods[methodName];

		introspection.push('<method name="' + methodName + '">');

		// Arguments
		for (var index in method['in']) {
			var arg = method['in'][index];
			if (arg.name)
				introspection.push('<arg direction="in" type="' + arg.type + '" name="' + arg.name + '"/>');
			else
				introspection.push('<arg direction="in" type="' + arg.type + '"/>');
		}

		if (method['out']) {
			if (method['out'].name)
				introspection.push('<arg direction="out" type="' + method['out'].type + '" name="' + method['out'].name + '"/>');
			else
				introspection.push('<arg direction="out" type="' + method['out'].type + '"/>');
		}

		introspection.push('</method>');
	}

	// Properties
	for (var propName in self.properties) {
		var property = self.properties[propName];

		introspection.push('<property name="' + propName + '" type="' + property['type'].type + '" access="' + property['access'] + '"/>');
	}

	// Signal
	for (var signalName in self.signals) {
		var signal = self.signals[signalName];

		introspection.push('<signal name="' + signalName + '">');

		// Arguments
		if (signal.types) {
			for (var index in signal.types) {
				var arg = signal.types[index];
				if (arg.name)
					introspection.push('<arg type="' + arg.type + '" name="' + arg.name + '"/>');
				else
					introspection.push('<arg type="' + arg.type + '"/>');
			}
		}

		introspection.push('</signal>');
	}

	introspection.push('</interface>');

	self.introspection = introspection.join('\n');

	return self;
};

var ServiceObject = function(service, objectPath) {
	var self = this;

  self.service = service;
	self.path = objectPath;
	self.interfaces = {};
	self.introspection = null;

	// Initializing introspectable interface
	self.introspectableInterface = self.createInterface('org.freedesktop.DBus.Introspectable');
	self.introspectableInterface.addMethod('Introspect', { out: Utils.Define(String, 'data') }, function(callback) {
		self.updateIntrospection();
		callback(null, self.introspection);
	});
	self.introspectableInterface.update();

	// Initializing property interface
	self.propertyInterface = self.createInterface('org.freedesktop.DBus.Properties');
	self.propertyInterface.addMethod('Get', {
		in: [
			Utils.Define(String, 'interface_name'),
			Utils.Define(String, 'property_name')
		],
		out: Utils.Define('Auto', 'value')
	}, function(interfaceName, propName, callback) {
		var iface = self['interfaces'][interfaceName];
		if (!iface) {
			callback(new Error('Doesn\'t support such property'));
			return;
		}

		if (!iface.getProperty(propName, callback))
			callback(new Error('Doesn\'t support such property'));
	});

	self.propertyInterface.addMethod('Set', {
		in: [
			Utils.Define(String, 'interface_name'),
			Utils.Define(String, 'property_name'),
			Utils.Define('auto', 'value')
		]
	}, function(interfaceName, propName, value, callback) {
		var iface = self['interfaces'][interfaceName];
		if (!iface) {
			callback(new Error('Doesn\'t support such property'));
			return;
		}

		if (!iface.setProperty(propName, value, callback))
			callback(new Error('Doesn\'t support such property'));
	});

	self.propertyInterface.addMethod('GetAll', {
		in: [
			Utils.Define(String, 'interface_name')
		],
		out: Utils.Define(Object, 'properties')
	}, function(interfaceName, callback) {
		var iface = self['interfaces'][interfaceName];
		if (!iface) {
			callback(new Error('Doesn\'t have any properties'));
			return;
		}

		iface.getProperties(function(err, props) {
			callback(err, props);
		});
	});
	
	self.propertyInterface.addSignal('PropertiesChanged', {
		types: [Utils.Define(String, 'interface_name'), Utils.Define(Object, 'changed_properties'), Utils.Define(Array, 'invalidated_properties')]
	});
	
	self.propertyInterface.update();
};

ServiceObject.prototype.createInterface = function(interfaceName) {
	var self = this;

	if (!self.interfaces[interfaceName])
		self.interfaces[interfaceName] = new ServiceInterface(self, interfaceName);

	self.interfaces[interfaceName].update();

	return self.interfaces[interfaceName];
};

ServiceObject.prototype.updateIntrospection = function() {
	var self = this;
	var i;

	var introspection = [
		'<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"',
		'"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">',
		'<node name="' + self.path + '">'
	];

	for (var interfaceName in self.interfaces) {
		var iface = self.interfaces[interfaceName];
		introspection.push(iface.introspection);
	}

	var childNodes = self.buildChildNodes();

	for (i = 0; i < childNodes.length; i++) {
		introspection.push('<node name="' + childNodes[i] + '"/>');
	}
	introspection.push('</node>');

	self.introspection = introspection.join('\n');
};

ServiceObject.prototype.buildChildNodes = function() {
	var self = this;

	function unique(arr) {
		var t = {};
		var i;
		for (i = 0; i < arr.length; i++) {
			t[arr[i]] = true;
		}

		return Object.keys(t);
	}

	var prefix = self.path + '/';
	var allKeys = Object.keys(self.service.objects);
	var childKeys = allKeys.filter(function(key) {
		return key.substr(0, prefix.length) == prefix
		    && key.length > prefix.length;
	});
	var unprefixedChildKeys = childKeys.map(function(key) {
		return key.substr(prefix.length);
	});
	var childNodes = unprefixedChildKeys.map(function(key) {
		return key.split('/')[0];
	});
	var uniqueChildNodes = unique(childNodes);

	return uniqueChildNodes;
};

var Service = function(bus, serviceName) {
	var self = this;

	self.bus = bus;
	self.serviceName = serviceName;
	self.objects = {};

	self.on('request', function(uniqueName, sender, objectPath, interfaceName, member, message, args) {
		var iface = self.objects[objectPath]['interfaces'][interfaceName];
		if (!iface)
			return;

		iface.call.apply(iface, [ member, message, args ]);
	});
};

util.inherits(Service, events.EventEmitter);

Object.defineProperty(Service.prototype, 'connected', {
	get: function() {
		return this.bus.connected;
	}
});

Service.prototype.createObject = function(objectPath) {
	var self = this;

	if (!self.objects[objectPath])
		self.objects[objectPath] = new ServiceObject(self, objectPath);

	// Register object
	self.bus._dbus.registerObjectPath(self.bus.connection, objectPath);

	return self.objects[objectPath];
};

Service.prototype.removeObject = function(object) {
	var self = this;

	self.bus._dbus.unregisterObjectPath(self.bus.connection, object.path);

	delete self.objects[object.path];
};

Service.prototype.disconnect = function() {
	var self = this;

	self.bus.disconnect();
};

var serviceMap = {};

// Dispatch events to service
_dbus.setObjectHandler(function(uniqueName, sender, objectPath, interfaceName, member, message, args) {

	for (var hash in serviceMap) {
		var service = serviceMap[hash];

		if (service.bus.connection.uniqueName != uniqueName)
			continue;

		// Fire event
		var newArgs = [ 'request' ].concat(Array.prototype.slice.call(arguments));
		service.emit.apply(service, newArgs);

		break;
	}
});

var DBus = function(opts) {
};

DBus.Define = Utils.Define;
DBus.Signature = Utils.Signature;
DBus.Error = DBusError;

DBus.getBus = function(busName) {
	return new Bus(_dbus, DBus, busName);
};

/* Deprecated */
DBus.prototype.getBus = function(busName) {
	return DBus.getBus(busName);
};

DBus.signalHandlers = {};

DBus.enableSignal = function(conn) {
	_dbus.setSignalHandler(conn, function(uniqueName) {

		if (DBus.signalHandlers[uniqueName]) {
			var args = [ 'signal' ].concat(Array.prototype.slice.call(arguments));

			DBus.signalHandlers[uniqueName].emit.apply(DBus.signalHandlers[uniqueName], args);
		}
	});
};

DBus.registerService = function(busName, serviceName) {

	var _serviceName = serviceName || null;

	if (serviceName) {

		// Return bus existed
		var serviceHash = busName + ':' + _serviceName;
		if (serviceMap[serviceHash])
			return serviceMap[serviceHash];
	}

	// Get a connection
	var bus = DBus.getBus(busName);

	if (!serviceName)
		_serviceName = bus.connection.uniqueName;

	// Create service
	var service = new Service(bus, _serviceName);
	serviceMap[serviceHash] = service;

	if (serviceName) {
		DBus._requestName(bus, _serviceName);
	}

	return service
};

/* Deprecated */
DBus.prototype.registerService = function(busName, serviceName) {
	return DBus.registerService(busName, serviceName);
};

DBus._requestName = function(bus, serviceName) {
	_dbus.requestName(bus.connection, serviceName);
};

export default DBus;
