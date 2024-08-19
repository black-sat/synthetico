import click

### Generation of PPLTL formula + partition file for triangle-tireworld domain (non-deterministic). Amount of layers can be altered
### Based on the translation of triangle_tireworld to LTLf from https://github.com/renzoq/IMS_thesis/blob/main/Python%20domain%20generation/triangle_tireworld_ltlf_generator.ipynb

# give number of layers as input
number_of_layers = []
states = []
node_names = []
paths = []
move_car_list = []


def generate_states(layers):
    all_states = []

    # Generate states for each layer
    for layer in range(1, layers + 1):
        current_layer_states = [f'{i}{layer - i + 1}' for i in range(1, layer + 1)]
        all_states.append(current_layer_states)
    return all_states


def print_and_return_states(all_states):
    node_names = []
    for layer in all_states:
        #print("layer: ", layer)
        for location in layer:
            node_names.append(location)
    return node_names


def get_change_tire_actions(nodes):
    action_list = []
    for node in nodes:
        action_list.append(("changetire_" + node))
    return action_list


def get_spare_tire_states(nodes):
    action_list = []
    for node in nodes:
        action_list.append(("sparein_" + node))
    return action_list


def get_vehicle_at_states(nodes):
    action_list = []
    for node in nodes:
        action_list.append(("vehicleat_" + node))
    return action_list


def get_paths_and_movecar(all_states):
    global paths
    global move_car_list
    
    paths = []
    move_car_list = []

    for i, layer in enumerate(all_states):
        for j, location in enumerate(layer):
            # try to add path to right node
            if j + 1 < len(layer):
                paths.append("road_" + str(all_states[i][j]) + "_" + str(all_states[i][j + 1]))
            # try to add path to left node
            if j != 0:
                paths.append("road_" + str(all_states[i][j]) + "_" + str(all_states[i][j - 1]))
            # try to add path to top right node
            if i != 0:
                try:
                    paths.append("road_" + str(all_states[i][j]) + "_" + str(all_states[i - 1][j]))
                except:
                    pass
                if j != 0:
                    # try to add path to top left node
                    try:
                        paths.append("road_" + str(all_states[i][j]) + "_" + str(all_states[i - 1][j - 1]))
                    except:
                        pass
            # try to add path to bottom right node
            if i != len(all_states):
                try:
                    paths.append("road_" + str(all_states[i][j]) + "_" + str(all_states[i + 1][j + 1]))
                except:
                    pass
            # try to add path to bottom left node
            try:
                paths.append("road_" + str(all_states[i][j]) + "_" + str(all_states[i + 1][j]))
            except:
                pass

    # IF ANY ROADS NEED TO BE REMOVED, CAN BE DONE IN LINE BELOW. Moves allowed by agent are added dynamically
    roads_to_remove = []

    paths = [road for road in paths if road not in roads_to_remove]

    for path in paths:
        action, from_l, to_l = path.split('_')
        move_car_list.append("movecar_" + from_l + "_" + to_l)

    return paths, move_car_list


def get_changable_fluents(nodes):
    changable_fluents = []
    changable_fluents.extend(get_spare_tire_states(nodes))
    changable_fluents.extend(get_vehicle_at_states(nodes))
    flattire = "flattire"
    changable_fluents.append(flattire)
    return changable_fluents


def get_mutual_exclusion_agent_actions(agent_actions):
    all_actions_or = " | ".join(agent_actions)
    action_combinations = []

    for i, action in enumerate(agent_actions):
        for j in range(i + 1, len(agent_actions)):
            action_combinations.append("!(" + agent_actions[i] + " & " + agent_actions[j] + ")")

    action_comb_str = " & ".join(action_combinations)

    return all_actions_or, action_comb_str

def get_agent_preconditions_PPLTL():
    prec_list = []
    prec_dictionary = {}
    # preconditions for 'movecar'
    for move in move_car_list:
        action, from_l, to_l = move.split('_')
        prec_list.append(
            "(!(vehicleat_" + from_l + " & road_" + from_l + "_" + to_l + " & !flattire) -> X(!" + move + "))")
        prec_dictionary[move] = "(vehicleat_" + from_l + " & road_" + from_l + "_" + to_l + " & !flattire)"

    # TODO: check if its not necessary to have a flat tire before changing it
    # preconditions for 'changetire'
    for change_act in get_change_tire_actions(node_names):
        action, from_l = change_act.split('_')
        prec_list.append("(!(sparein_" + from_l + " & vehicleat_" + from_l + " & flattire) -> X(!" + change_act + "))")
        prec_dictionary[change_act] = "(sparein_" + from_l + " & vehicleat_" + from_l + " & flattire)"

    return prec_dictionary

def get_environment_transitions():
    effect_list = []
    # effect 'movecar'
    for move in move_car_list:
        action, from_l, to_l = move.split('_')
        effect_list.append(
            "(vehicleat_" + from_l + " -> X(" + move + " -> (vehicleat_" + to_l + " & (flattire | !flattire))))")

    # effect 'changetire'
    for change_act in get_change_tire_actions(node_names):
        action, from_l = change_act.split('_')
        effect_list.append(
            "(sparein_" + from_l + " & vehicleat_" + from_l + " -> X(" + change_act + " -> (!sparein_" + from_l + " & !flattire & vehicleat_" + from_l + ")))")

    return effect_list

def get_environment_add_del():
    fluents = get_changable_fluents(node_names)
    add_list = {}
    del_list = {}
    for fluent in fluents:
        add_list[fluent] = []
        del_list[fluent] = []
    for fluent in fluents:
        for move in move_car_list:
            action, from_l, to_l = move.split('_')
            from_l = "vehicleat_"+ from_l
            to_l = "vehicleat_" + to_l
            if fluent == from_l:
                del_list[fluent].append(move)
            if fluent == to_l:
                add_list[fluent].append(move)

        # effect 'changetire'
        for change_act in get_change_tire_actions(node_names):
            action, from_l = change_act.split('_')
            if fluent == "sparein_" + from_l:
                del_list[fluent].append(change_act)
            if fluent == "flattire":
                del_list[fluent].append(change_act)

    return add_list, del_list


def get_environment_transitions_PPLTL():
    formula_list = []
    prec_dictionary = get_agent_preconditions_PPLTL()
    add_list, del_list = get_environment_add_del()
    fluents = get_changable_fluents(node_names)
    for fluent in fluents:
        add_action_precondition_list = []
        for add_action in add_list[fluent]:
            add_action_precondition_list.append("(" + add_action + " & " + prec_dictionary[add_action] + ")")
        del_action_precondition_list = []
        for del_action in del_list[fluent]:
            del_action_precondition_list.append("(" + del_action + " & " + prec_dictionary[del_action] + ")")

        activated_by_add_actions = " | ".join(add_action_precondition_list)
        deactivated_by_del_actions = " | ".join(del_action_precondition_list)

        if activated_by_add_actions == "":
            formula_list.append("(" + fluent + " <-> " + "Y(" + fluent + " & (!(" + deactivated_by_del_actions + "))))")
        else:
            if deactivated_by_del_actions == "":
                formula_list.append("(" + fluent + " <-> " + "(Y(" + activated_by_add_actions + ")))")
            else:
                formula_list.append("(" + fluent + " <-> " + "(Y(" + activated_by_add_actions + ")" + " | " + "Y(" + fluent + " & (!(" + deactivated_by_del_actions + ")))))")

    return formula_list


# use this for specifying which fluents never change during a run (like roads)
def get_consistent_fluents(fluents):
    return "G(" + " & ".join(fluents) + ")"

def get_consistent_fluents_PPLTL(fluents):
    return "H(" + " & ".join(fluents) + ")"


def get_next_spare_status():
    sparetires = get_spare_tire_states(node_names)
    sparenext = []

    for spare in sparetires:
        action, location = spare.split('_')
        sparenext.append("(!" + spare + " -> X(!" + spare + "))")
        sparenext.append("((" + spare + " & !changetire_" + location + ") -> X(" + spare + "))")

    return "G(" + " & ".join(sparenext) + ")"


def get_agent_environment_actions():
    paths, move_car_list = get_paths_and_movecar(states)

    environment_actions = get_vehicle_at_states(node_names) + get_spare_tire_states(node_names) + paths
    agent_actions = move_car_list + get_change_tire_actions(node_names)

    return environment_actions, agent_actions


### the goal and initialization have to be set by hand (these are hardcoded):
# this has to be set by hand
def get_goal():
    return "O(vehicleat_21)"


# this has to be set by hand
def get_initialization():
    init_list = ["!flattire", "vehicleat_11", "sparein_12", "sparein_11", "sparein_21"]

    return " & ".join(init_list)


@click.command()
@click.argument('layers', type=int)
def main(layers: int):
    global number_of_layers
    global states
    global node_names

    number_of_layers = layers

    states = generate_states(number_of_layers)
    node_names = print_and_return_states(states)

    # print("environment locations: ", node_names)
    # print()
    # print("change tire actions: ", get_change_tire_actions(node_names))
    # print()
    # print("spare tires: ", get_spare_tire_states(node_names))
    # print()
    # print("vehicle at: ", get_vehicle_at_states(node_names))
    # print()
    # paths, move_car_list = get_paths_and_movecar(states)
    # print("roads: ", paths)
    # print()
    # print("move car options: ", move_car_list)
    # print()
    # flattire = "flattire"
    # print("flattire: ", flattire)
    # print("changable fluents: ", get_changable_fluents(node_names))
    # print()

    ### code block below retrieves the mutual exclusion of agent actions and specific environment fluents. Also agent preconditions are retrieved:
    # 'mutual_excl_all' contains the or-statement between all actions
    # 'mutual_excl_comb' contains the negation of all possible agent action combinations
    env_act_list, agent_act_list = get_agent_environment_actions()

    mutual_excl_all, mutual_excl_comb = get_mutual_exclusion_agent_actions(agent_act_list)
    mutual_excl_all_env, mutual_excl_comb_env = get_mutual_exclusion_agent_actions(get_vehicle_at_states(node_names))

    # print("pre_condition:", get_agent_preconditions_PPLTL())

    add_list, del_list = get_environment_add_del()
    # print("add_list: ", add_list)
    # print("del_list: ", del_list)

    # # init
    # print("(", get_initialization(), ")", sep='')
    # # agent
    # print("((", mutual_excl_all, ") & ", mutual_excl_comb, ")", sep='')
    # # environment
    # print("(((", mutual_excl_all_env, ") & ", mutual_excl_comb_env, ") & (", " & ".join(get_environment_transitions()),
    #     ") & (", get_next_spare_status(), ") & ", get_consistent_fluents(paths), ")", sep='')

    # # goal
    # print(get_goal(), sep='')

    # print("--------------------------------")

    # complete formula, written as follows: (H(phi_agent_unique_act) & ((phi_env_init & H(phi_env_transitions_PPLTL)) -> (phi_goal)))
    print("'F(H((", mutual_excl_all, ") & ", mutual_excl_comb, ") & ((", get_initialization(), ") & H(((",
        mutual_excl_all_env, ") & ", mutual_excl_comb_env, ") & (", " & ".join(get_environment_transitions_PPLTL()), ") & ",
        get_consistent_fluents_PPLTL(paths), ")) -> (", get_goal(), "))'", 
        ' ', " ".join(env_act_list), " flattire", sep='')

    # print out partition file
    # print(".inputs:\n", " ".join(env_act_list), " flattire", sep='')
    # print(".outputs:\n", " ".join(agent_act_list), sep='')  

if __name__ == '__main__':
    main()