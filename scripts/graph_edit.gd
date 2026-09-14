extends GraphEdit

const COLOR_NODE_RESOURCE := preload("res://scenes/color_node.tscn")
const IMAGE_NODE_RESOURCE := preload("res://scenes/image_node.tscn")
const GRAY_NODE_RESOURCE := preload("res://scenes/gray_node.tscn")
const DISPLAY_NODE_RESOURCE := preload("res://scenes/display_node.tscn")

var selected_nodes: Dictionary
var popup_menu: PopupMenu
var file_dialog: FileDialog

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	selected_nodes = {}
	file_dialog = get_node("FileDialog")
	popup_menu = get_node("PopupMenu")

	popup_menu.add_item("Color", 1)
	popup_menu.add_item("Image", 2)
	popup_menu.add_item("Grayscale", 3)
	popup_menu.add_item("Display", 4)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	pass

func _input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_graph_delete"):
		for node: Node in selected_nodes.keys():
			if selected_nodes[node]:
				for connection in get_connection_list_from_node(node.name):
					disconnect_node(connection["from_node"], connection["from_port"], connection["to_node"], connection["to_port"])
				node.queue_free()
		selected_nodes.clear()
	
	if event.is_action_pressed("context_menu"):
		popup_menu.popup(Rect2i(Vector2i(get_viewport().get_mouse_position()), Vector2i(0, 0)))


func _on_popup_menu_id_pressed(id: int) -> void:
	match id:
		1:
			var node: GraphNode = COLOR_NODE_RESOURCE.instantiate()
			node.position_offset = get_viewport().get_mouse_position()
			add_child(node)
		2:
			file_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
			file_dialog.filters = PackedStringArray(["*.png,*.jpg,*.jpeg", "Image file"])
			file_dialog.popup()
		3:
			var node: GraphNode = GRAY_NODE_RESOURCE.instantiate()
			node.position_offset = get_viewport().get_mouse_position()
			add_child(node)
		4:
			var node: GraphNode = DISPLAY_NODE_RESOURCE.instantiate()
			node.position_offset = get_viewport().get_mouse_position()
			add_child(node)

func _on_file_dialog_file_selected(path: String) -> void:
	var image = Image.load_from_file(path)
	var node: GraphNode = IMAGE_NODE_RESOURCE.instantiate()
	var texture_rect: TextureRect = node.get_node("TextureRect")
	var image_size := image.get_size()

	# Extend texture rect based on image ratio
	if image_size.x > image_size.y:
		texture_rect.custom_maximum_size.x = texture_rect.custom_maximum_size.x as float * image_size.x as float / image_size.y as float
	else:
		texture_rect.custom_maximum_size.y = texture_rect.custom_maximum_size.y as float * image_size.y as float / image_size.x as float

	texture_rect.texture = ImageTexture.create_from_image(image)
	node.position_offset = get_viewport().get_mouse_position()
	add_child(node)


func _on_connection_request(from_node: StringName, from_port: int, to_node: StringName, to_port: int) -> void:
	for connection in get_connection_list_from_node(from_node):
		if connection["to_node"] == to_node and connection["to_port"] == to_port:
			return
	
	connect_node(from_node, from_port, to_node, to_port)


func _on_disconnection_request(from_node: StringName, from_port: int, to_node: StringName, to_port: int) -> void:
	disconnect_node(from_node, from_port, to_node, to_port)


func _on_node_selected(node: Node) -> void:
	selected_nodes[node] = true


func _on_node_deselected(node: Node) -> void:
	selected_nodes[node] = false
