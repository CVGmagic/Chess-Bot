extends Node


var chess_engine: ChessEngine
@onready var chess_board: Control = %ChessBoard

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	print("Starting initialisation")
	chess_engine = ChessEngine.new()
	print("Chess engine created successfully")
	set_engine_board(chess_board.board)
	print("Board initialised successfully")
	
	set_engine_board(chess_board.board)
	var move: Array[int] = decode_move(chess_engine.get_random_legal_move())
	chess_board.make_move(move[0], move[1], move[2], move[3])

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


func make_random_engine_move() -> void:
	var move: Array[int] = decode_move(chess_engine.get_random_legal_move())
	chess_board.make_move(move[0], move[1], move[2], move[3])
	print("Engine Move: (" + str(move[0]) + "," + str(move[1]) + ") (" + str(move[2]) + "," + str(move[3]) + ")")
	
	
func set_engine_board(board: Array[Array]) -> void:
	var flat_array: PackedInt32Array = PackedInt32Array()
	flat_array.resize(64) # Allocate all 64 squares upfront for speed
	
	for rank in range(8):
		for file in range(8):
			flat_array[rank * 8 + file] = board[rank][file]
	
	chess_engine.set_board_from_array(flat_array, 0, -1)


func try_move(from_rank: int, from_file: int, to_rank: int, to_file: int) -> bool:
	var success: bool = chess_engine.try_move(from_rank, from_file, to_rank, to_file, 4)
	# promo choice set to 4 (queen) by default
	return success
	
	
func decode_move(move: int) -> Array[int]:
	var from_sq_num: int = move & 63
	var to_sq_num: int = (move >> 6) & 63
	var flag: int = (move >> 12) & 15
	
	var from_rank: int = int(from_sq_num / 8)
	var from_file: int = from_sq_num % 8
	var to_rank: int = int(to_sq_num / 8)
	var to_file: int = to_sq_num % 8
	
	return [from_rank, from_file, to_rank, to_file, flag]
