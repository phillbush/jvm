public class Multi {
	public static void main(String[] args) {
		String[][] arr = new String[10][10];

		arr[0][0] = "test1";
		arr[0][1] = "test2";
		arr[9][5] = "test3";
		System.out.println(arr[0][0]);
		System.out.println(arr[0][1]);
		System.out.println(arr[9][5]);
	}
}
