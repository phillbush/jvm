public class StringTest {
	static final String str = "Hello world!";
	public static void main(String[] args) {
		for (int i = 0; i < str.length(); i++) {
			System.out.print(str.charAt(i));
		}
		System.out.println("");
	}
}
