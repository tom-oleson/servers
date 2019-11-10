



<pre>
// Useful in node-red:
//
// Parse string into object and add name
// arduino001:{"time":157340841}
// becomes:
// {"time":147340841, "name":"arduino001"}

var str = msg.payload.toString();
try { 
    var n = str.indexOf(":");
    var obj = JSON.parse(str.substr(n+1)); 
    var name = str.substr(0, n);
    if(name.length > 0) {
        obj["name"] = name;
    }
} catch(e) {
    node.error(str);
}

return obj;
</pre>
