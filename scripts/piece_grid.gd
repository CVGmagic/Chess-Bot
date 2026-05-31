extends GridContainer


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
	

var pieces: Array[Array] = []

func generate_pieces(board: Array[Array], piece_map: Dictionary[int, String], board_width: int=560):
	var num_rows: int = board.size()
	var num_columns: int = board[0].size()
	
	# Create empty pieces array
	pieces.resize(num_rows)
	for i in range(num_rows):
		pieces[i] = []
		pieces[i].resize(num_columns)
		pieces[i].fill(null)
		
		
	for rank in range(num_rows):
		for file in range(num_columns):
			if board[rank][file] == 0:
				continue
				
			# Create a flat button or simple panel for each square
			var piece: Sprite2D = Sprite2D.new()
			
			# Force each square to be a uniform square size (e.g., 70x70 pixels)
			var square_width: int = floor(board_width / num_columns)
			var square_height: int = floor(board_width / num_rows)
			
			var file_path: String = "res://assets/pieces/" + piece_map[board[rank][file]] + ".svg"
			piece.texture = load(file_path)
			
			# Name the square node by its algebraic notation coordinates for debugging
			
			piece.position = Vector2(square_width * file + square_width / 2, square_height * rank + square_height / 2)
			pieces[rank][file] = piece
			
			# Add the square to our GridContainer
			add_child(piece)


func move_piece(cur_square: Vector2i, target_square: Vector2i):
	pieces[target_square.x][target_square.y] = pieces[cur_square.x][cur_square.y]
	pieces[cur_square.x][cur_square.y]
