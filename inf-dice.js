function disable_display(obj){
    var el = document.getElementById(obj);

    el.style.display = "none";
}

function enable_display(obj){
    var el = document.getElementById(obj);

    el.style.display = "";
}

function toggle_display(obj){
    var el = document.getElementById(obj);

    if(el.style.display == "none"){
        el.style.display = "";
    }else{
        el.style.display = "none";
    }
}

function _set_style_recursive(obj, styles){
    if("style" in obj){
        for(var i = 0; i < styles.length; i++){
            obj.style.setProperty(styles[i][0], styles[i][1]);
        }

        var children = obj.childNodes;

        for(var i = 0; i < children.length; i++){
            _set_style_recursive(children[i], styles);
        }
    }
}

function enable_input(id){
    var obj = document.getElementById(id);
    var styles = [
        ["color", "black"],
    ];

    _set_style_recursive(obj, styles);
}

function disable_input(id){
    var obj = document.getElementById(id);
    var styles = [
        ["color", "#555555"],
    ];

    _set_style_recursive(obj, styles);
}

function other_player(player){
    if(player == "p1"){
        return "p2";
    }else{
        return "p1";
    }
}

function set_ammo(player, check_params){
    var other = other_player(player);

    var action_name = player + ".action";
    var action = document.getElementsByName(action_name)[0];

    var ammo;
    if(action.value == "bs" || action.value == "cc" || action.value == "dtw" || action.value == "throw"){
        ammo = document.getElementsByName(player + ".ammo")[0].value;
    }else{
        // Any case where we aren't attacking
        ammo = "None";
    }

    var arm_id = other + ".arm";
    var bts_id = other + ".bts";
    var dam_id = player + ".dam";

    if(ammo == "Smoke" || ammo == "None"){
        disable_input(dam_id);
        disable_input(arm_id);
        disable_input(bts_id);
    }else if(ammo == "Monofilament" || ammo == "K1"){
        disable_input(dam_id);
        disable_input(arm_id);
        disable_input(bts_id);
    }else if(ammo == "Viral" || ammo == "E/M" || ammo == "E/M2" || ammo == "Nanotech"){
        enable_input(dam_id);
        enable_input(bts_id);
        disable_input(arm_id);
    }else{
        enable_input(dam_id);
        enable_input(arm_id);
        disable_input(bts_id);
    }

    // Set B values based on selected weapon and ammo
    var weapon_name = document.getElementsByName(player + ".weapon")[0].value;
    var weapon = weapon_data[weapon_name];
    var b_list = document.getElementsByName(player + ".b")[0];

    // Get ammo type index so we can look up the corresponding B value
    b_list.length = 0;
    if(weapon){
        for(var i = 0; i < weapon["ammo"].length; i++){
            if(ammo == weapon["ammo"][i]){
                var b_selected;
                for(var b = 1; b <= weapon["b"][i]; b++){
                    b_list.options[b_list.options.length] = new Option(b);

                    if(check_params && b == params[player + ".b"]){
                        b_list.options[b_list.options.length - 1].selected = true;
                        b_selected = true;
                    }
                }

                if(!b_selected && player == "p1" && action.value != "cc"){
                    // Default to the highest burst for Player 1
                    b_list.options[b_list.options.length - 1].selected = true;
                }
                break;
            }
        }
    }else{
        // Custom Weapon
        for(var b = 1; b <= 5; b++){
            b_list.options[b_list.options.length] = new Option(b);

            if(check_params && b == params[player + ".b"]){
                b_list.options[b_list.options.length - 1].selected = true;
            }
        }
    }
}

// helper function for set_action
// berserk works in CC vs. CC, Dodge, or None
function set_berserk(player, other){
    var action_name = player + ".action";
    var action = document.getElementsByName(action_name)[0].value;
    var other_action_name = other + ".action";
    var other_action = document.getElementsByName(other_action_name)[0].value;

    if(action == "cc" && other_action == "cc"){
        enable_input(player + ".berserk");
        enable_input(other + ".berserk");
    }else if(action == "cc" && (other_action == "dodge" || other_action == "none")){
        enable_input(player + ".berserk");
        disable_input(other + ".berserk");
    }else if(other_action == "cc" && (action == "dodge" || action == "none")){
        disable_input(player + ".berserk");
        enable_input(other + ".berserk");
    }else{
        disable_input(player + ".berserk");
        disable_input(other + ".berserk");
    }
}

function set_action(player){
    var other = other_player(player);
    var action_name = player + ".action";
    var action = document.getElementsByName(action_name)[0];
    var other_action_name = other + ".action";
    var other_action = document.getElementsByName(other_action_name)[0];

    if(action.value == "bs" || action.value == "throw"){
        // stat block
        if(action.value == "bs"){
            enable_input(player + ".bs");
            disable_input(player + ".ph");
        }else if(action.value == "throw"){
            disable_input(player + ".bs");
            enable_input(player + ".ph");
        }
        disable_input(player + ".cc");
        disable_input(player + ".wip");
        enable_input(player + ".b");
        enable_input(player + ".ammo");

        // modifiers
        enable_input(player + ".range");
        enable_input(player + ".link");
        enable_input(player + ".viz");
        disable_input(player + ".motorcycle");

        // cc modifiers
        disable_input(player + ".gang_up");
        disable_input(other + ".ikohl");
        set_berserk(player, other);

        // defensive abilities
        enable_input(other + ".cover");
        enable_input(other + ".ch");
        disable_input(player + ".hyperdynamics");

        // ability sections
        enable_display(player + ".sec_weapon");
        enable_display(player + ".sec_shoot");
        disable_display(player + ".sec_cc");
    }else if(action.value == "dtw"){
        // stat block
        disable_input(player + ".bs");
        disable_input(player + ".ph");
        disable_input(player + ".cc");
        disable_input(player + ".wip");
        enable_input(player + ".b");
        enable_input(player + ".ammo");

        // modifiers
        disable_input(player + ".range");
        enable_input(player + ".link");
        disable_input(player + ".viz");
        disable_input(player + ".motorcycle");

        // cc modifiers
        disable_input(player + ".gang_up");
        disable_input(other + ".ikohl");
        set_berserk(player, other);

        // defensive abilities
        enable_input(other + ".cover");
        disable_input(other + ".ch");
        disable_input(player + ".hyperdynamics");

        // ability sections
        enable_display(player + ".sec_weapon");
        enable_display(player + ".sec_shoot");
        disable_display(player + ".sec_cc");
    }else if(action.value == "cc"){
        // stat block
        disable_input(player + ".bs");
        disable_input(player + ".ph");
        enable_input(player + ".cc");
        disable_input(player + ".wip");
        disable_input(player + ".b");
        enable_input(player + ".ammo");

        // modifiers
        disable_input(player + ".range");
        disable_input(player + ".link");
        disable_input(player + ".viz");
        disable_input(player + ".motorcycle");

        // cc modifiers
        enable_input(player + ".gang_up");
        enable_input(other + ".ikohl");
        set_berserk(player, other);

        // defensive abilities
        disable_input(other + ".cover");
        disable_input(other + ".ch");
        disable_input(player + ".hyperdynamics");

        // ability sections
        enable_display(player + ".sec_weapon");
        disable_display(player + ".sec_shoot");
        enable_display(player + ".sec_cc");
    }else if(action.value == "dodge"){
        // stat block
        disable_input(player + ".bs");
        enable_input(player + ".ph");
        disable_input(player + ".cc");
        disable_input(player + ".wip");
        disable_input(player + ".b");
        disable_input(player + ".ammo");

        // modifiers
        disable_input(player + ".range");
        disable_input(player + ".link");
        disable_input(player + ".viz");
        enable_input(player + ".motorcycle");

        // cc modifiers
        enable_input(player + ".gang_up");
        disable_input(other + ".ikohl");
        set_berserk(player, other);

        // defensive abilities
        disable_input(other + ".cover");
        disable_input(other + ".ch");
        enable_input(player + ".hyperdynamics");

        // ability sections
        disable_display(player + ".sec_weapon");
        disable_display(player + ".sec_shoot");
        disable_display(player + ".sec_cc");
    }else if(action.value == "none"){
        // stat block
        disable_input(player + ".bs");
        disable_input(player + ".ph");
        disable_input(player + ".cc");
        disable_input(player + ".wip");
        disable_input(player + ".b");
        disable_input(player + ".ammo");

        // modifiers
        disable_input(player + ".range");
        disable_input(player + ".link");
        disable_input(player + ".viz");
        disable_input(player + ".motorcycle");

        // cc modifiers
        disable_input(player + ".gang_up");
        disable_input(other + ".ikohl");
        set_berserk(player, other);

        // defensive abilities
        disable_input(other + ".cover");
        disable_input(other + ".ch");
        disable_input(player + ".hyperdynamics");

        // ability sections
        disable_display(player + ".sec_weapon");
        disable_display(player + ".sec_shoot");
        disable_display(player + ".sec_cc");
    }
    populate_weapons(player);
}

function set_weapon(player, check_params){
    // Set ammo types based on the selected weapon
    var stat_list = document.getElementsByName(player + ".stat")[0];
    var ammo_list = document.getElementsByName(player + ".ammo")[0];
    var dam_list = document.getElementsByName(player + ".dam")[0];
    var weapon_name = document.getElementsByName(player + ".weapon")[0].value;
    var action = document.getElementsByName(player + ".action")[0].value;
    var weapon = weapon_data[weapon_name];

    // default selected values
    var selected_ammo = ammo_list.value;
    if(check_params){
        selected_ammo = params[player + ".ammo"];
    }
    var selected_damage = dam_list.value;
    if(check_params){
        selected_damage = params[player + ".dam"];
    }
    var selected_stat = stats.value;
    if(check_params){
        selected_stat = params[player + ".stat"];
    }

    if(weapon){
        // Ammo types
        ammo_list.length = 0;
        for(var i = 0; i < weapon["ammo"].length; i++){
            ammo_list.options[i] = new Option(weapon["ammo"][i]);

            if(weapon["ammo"][i] == selected_ammo){
                ammo_list.options[i].selected = true;
            }
        }

        // Stat used
        stat_list.length = 0;
        var stat = weapon["stat"];
        if(action == "cc"){
            stat = "CC";
        }else if(action == "dtw"){
            stat = "--";
        }
        stat_list.options[0] = new Option(stat);

        dam_list.length = 0;
        dam_list.options[0] = new Option(weapon["dam"]);
    }else{
        // Custom weapon
        // set default values

        ammo_list.length = 0;
        for(var i = 0; i < ammos.length; i++){
            ammo_list.options[i] = new Option(ammos[i]);

            if(ammos[i] == selected_ammo){
                ammo_list.options[i].selected = true;
            }
        }

        dam_list.length = 0;
        for(var i = 0; i < damages.length; i++){
            dam_list.options[i] = new Option(damages[i]);

            if(dam_list.options[i].value == selected_damage){
                dam_list.options[i].selected = true;
            }
        }

        stat_list.length = 0;
        if(action == "cc"){
            stat_list.options[0] = new Option("CC");
        }else if(action == "dtw" || action == "dodge"){
            stat_list.options[0] = new Option("--");
        }else{
            for(var i = 0; i < stats.length; i++){
                stat_list.options[i] = new Option(stats[i]);

                if(stats[i] == selected_stat){
                    stat_list.options[i].selected = true;
                }
            }
        }
    }

    set_ammo(player, check_params);
}

function populate_weapons(player, check_params){
    var weapon_list = document.getElementsByName(player + ".weapon")[0];
    var action = document.getElementsByName(player + ".action")[0].value;
    var unit = get_unit_data(player);

    var attack_filter = "att_" + action;

    var selected_weapon = weapon_list.value;
    if(check_params){
        selected_weapon = params[player + ".weapon"];
    }

    var weapons;
    if(unit){
        weapons = unit.weapons;
    }else{
        weapons = Object.keys(weapon_data);
    }

    weapon_list.length = 0;

    for(var i = 0; i < weapons.length; i++){
        var weapon = weapon_data[weapons[i]];
        if(weapon && weapon[attack_filter]){
            weapon_list.options[weapon_list.options.length] = new Option(weapon["name"]);

            if(weapon["name"] == selected_weapon){
                weapon_list.options[weapon_list.options.length - 1].selected = true;
            }
        }
    }

    weapon_list.options[weapon_list.options.length] = new Option("--");
    weapon_list.options[weapon_list.options.length - 1].disabled = true;

    if(action == "bs" || action == "cc" || action == "throw"){
        weapon_list.options[weapon_list.options.length] = new Option("Custom Weapon");

        if("Custom Weapon" == selected_weapon){
            weapon_list.options[weapon_list.options.length - 1].selected = true;
        }
    }

    set_weapon(player, check_params);
}

function get_unit_data(player){
    var faction_name = player + ".faction";
    var faction = document.getElementsByName(faction_name)[0].value;

    var unit_name = player + ".unit";
    var selected_unit = document.getElementsByName(unit_name)[0].value;

    for(var i = 0; i < unit_data[faction].length; i++){
        if(unit_data[faction][i]["name"] == selected_unit){
            return unit_data[faction][i];
        }
    }
}

function ch_mod(unit){
    if(unit["ch"] == 1){
        return -3;
    }else if(unit["ch"] == 2){
        return -3;
    }else if(unit["ch"] == 3){
        return -6;
    }else if(unit["odd"]){
        return -6;
    }
    return 0;
}

function full_stat_dropdown(player, stat, min, max, check_params){
    var list = document.getElementsByName(player + "." + stat)[0];
    var selected = list.value;
    if(check_params){
        selected = params[player + "." + stat];
    }

    list.length = 0;

    for(var s = min; s <= max; s++){
        list.options[list.options.length] = new Option(s);

        if(s == selected){
            list.options[list.options.length - 1].selected = true;
        }
    }
}

function full_stat_dropdown_list(player, stat, stat_list, check_params){
    var list = document.getElementsByName(player + "." + stat)[0];
    var selected = list.value;
    if(check_params){
        selected = params[player + "." + stat];
    }

    list.length = 0;

    for(var s = 0; s < stat_list.length; s++){
        list.options[list.options.length] = new Option(stat_list[s]);

        if(stat_list[s] == selected){
            list.options[list.options.length - 1].selected = true;
        }
    }
}

function selected_stat_dropdown(player, stat, unit){
    var list = document.getElementsByName(player + "." + stat)[0];
    list.length = 0;

    list.options[0] = new Option(unit[stat]);
}

function set_unit(player, check_params){
    var unit = get_unit_data(player);

    // set all attributes from this unit
    if(unit){
        disable_display(player + ".attributes");
        enable_display(player + ".skills");

        // stat block
        selected_stat_dropdown(player, "cc", unit);
        selected_stat_dropdown(player, "bs", unit);
        selected_stat_dropdown(player, "ph", unit);
        selected_stat_dropdown(player, "wip", unit);
        selected_stat_dropdown(player, "arm", unit);
        selected_stat_dropdown(player, "bts", unit);
        selected_stat_dropdown(player, "w", unit);
        selected_stat_dropdown(player, "w_type", unit);
        selected_stat_dropdown(player, "type", unit);

        // list of skills
        var skills = document.getElementById(player + ".statline_skills");
        skills.innerHTML = unit["spec"].join(", ");

        // abilities
        document.getElementsByName(player + ".ikohl")[0].value = unit["ikohl"];
        document.getElementsByName(player + ".immunity")[0].value = unit["immunity"];
        document.getElementsByName(player + ".hyperdynamics")[0].value = unit["hyperdynamics"];
        document.getElementsByName(player + ".ch")[0].value = ch_mod(unit);
        document.getElementsByName(player + ".msv")[0].value = unit["msv"];
        document.getElementsByName(player + ".symbiont")[0].value = unit["symbiont"];
        document.getElementsByName(player + ".operator")[0].value = unit["operator"] || 0;

        document.getElementsByName(player + ".nwi")[0].checked = unit["nwi"];
        document.getElementsByName(player + ".shasvastii")[0].checked = unit["shasvastii"];
        document.getElementsByName(player + ".motorcycle")[0].checked = unit["motorcycle"];
    }else{
        // If they selected custom unit
        enable_display(player + ".attributes");
        disable_display(player + ".skills");

        // stat block
        full_stat_dropdown(player, "cc", 1, stat_max, check_params);
        full_stat_dropdown(player, "bs", 1, stat_max, check_params);
        full_stat_dropdown(player, "ph", 1, stat_max, check_params);
        full_stat_dropdown(player, "wip", 1, stat_max, check_params);
        full_stat_dropdown(player, "arm", 1, arm_max, check_params);
        full_stat_dropdown(player, "w", 1, w_max, 1, check_params);
        full_stat_dropdown_list(player, "bts", bts_list, check_params);
        full_stat_dropdown_list(player, "w_type", w_types, check_params);
        full_stat_dropdown_list(player, "type", unit_types, check_params);
    }

    populate_weapons(player, check_params);
}

function set_faction(player, check_params){
    var faction_name = player + ".faction";
    var faction = document.getElementsByName(faction_name)[0].value;

    var unit_name = player + ".unit";
    var unit_list = document.getElementsByName(unit_name)[0];

    unit_list.options.length = 0;

    var type = "";
    var selected = false;

    for(var i = 0; i < unit_data[faction].length; i++){
        var unit = unit_data[faction][i];
        if(type != unit["type"]){
            type = unit["type"];
            unit_list.options[unit_list.options.length] = new Option("-- " + type);
            unit_list.options[unit_list.options.length - 1].disabled = true;
        }

        unit_list.options[unit_list.options.length] = new Option(unit["name"]);
        if(!selected && (!check_params || !params[player + ".unit"])){
            unit_list.options[unit_list.options.length - 1].selected = true;
            selected = true;
        }

        if(!selected && unit["name"] == params[player + ".unit"]){
            unit_list.options[unit_list.options.length - 1].selected = true;
            selected = true;
        }
    }

    unit_list.options[unit_list.options.length] = new Option("-- Custom");
    unit_list.options[unit_list.options.length - 1].disabled = true;
    unit_list.options[unit_list.options.length] = new Option("Custom Unit");

    if(!selected){
        unit_list.options[unit_list.options.length - 1].selected = true;
    }

    set_unit(player, check_params);
}

var params = {};
function parse_params(){
    var query = document.location.search;
    query = query.split("+").join(" ");

    var tokens, re = /[?&]?([^=]+)=([^&]*)/g;

    while (tokens = re.exec(query)) {
        params[decodeURIComponent(tokens[1])] = decodeURIComponent(tokens[2]);
    }
}

function init_on_load(){
    parse_params();

    set_action("p1");
    set_action("p2");

    set_faction("p1", true);
    set_faction("p2", true);
}

function raw_output(){
    toggle_display("raw_output");
}

var damages = ["PH-2", "PH", 10, 11, 12, 13, 14, 15];
var stats = ["BS", "PH", "WIP"];
var w_types = ["W", "STR"];
var unit_types = ["LI", "MI", "HI", "SK", "WB", "TAG", "REM"];
var bts_list = [0, -3, -6, -9];
var stat_max = 22;
var arm_max = 10;
var w_max = 3;
